/*
 * Question 5 : Evaluation d'un polynome en un point.
 *
 * Ce fichier reprend la Q4 et y AJOUTE une fonction
 *   double eval(POINTEUR p, double x);
 * qui calcule la valeur du polynome p pour la variable X = x.
 *
 * Methode : on parcourt la liste chainee et on accumule le total
 *   total = somme des (coef_i * x^expo_i)
 *
 * Pour eviter la dependance a math.h, on calcule x^n avec une
 * petite boucle de multiplications (n est un naturel, donc >= 0).
 *
 * Pour pouvoir tester, le programme accepte une valeur de x en
 * argument optionnel sur la ligne de commande :
 *   ./analyseur          -> affiche juste le polynome
 *   ./analyseur 2.5      -> affiche aussi P(2.5)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* ------------------------------------------------------------------
   Structure de donnees : un polynome est une liste chainee de
   maillons. Chaque maillon contient un coefficient (reel), un
   exposant (entier naturel) et un pointeur vers le maillon suivant.
   ------------------------------------------------------------------ */

typedef struct maillon {
    double          coef;
    int             expo;
    struct maillon *suivant;
} MAILLON;

typedef MAILLON *POINTEUR;

/* Alloue un nouveau maillon avec les valeurs donnees. En cas d'echec
   d'allocation, on arrete le programme. */
static POINTEUR nouveauMaillon(double coef, int expo, POINTEUR suivant) {
    POINTEUR m = (POINTEUR) malloc(sizeof(MAILLON));
    if (m == NULL) {
        fprintf(stderr, "Erreur : memoire insuffisante\n");
        exit(EXIT_FAILURE);
    }
    m->coef = coef;
    m->expo = expo;
    m->suivant = suivant;
    return m;
}

/* Insere le maillon m dans la liste triee 'liste' (degres decroissants)
   et renvoie la nouvelle tete de la liste.

   Trois cas se presentent selon l'exposant de m compare a celui du
   premier maillon :
     - liste vide ou expo de m strictement plus grand : m devient la
       nouvelle tete.
     - meme exposant : on FUSIONNE en additionnant les coefficients
       sur le maillon existant ; si le total devient nul, on retire
       ce maillon.
     - expo de m strictement plus petit : on descend dans la queue
       par appel recursif.

   Le maillon m peut devenir orphelin si la fusion l'absorbe ; on ne
   le libere pas, conformement a l'esprit du projet (recyclage en Q7).
*/
static POINTEUR insererMaillon(POINTEUR liste, POINTEUR m) {
    if (liste == NULL || liste->expo < m->expo) {
        m->suivant = liste;
        return m;
    }
    if (liste->expo == m->expo) {
        liste->coef += m->coef;
        if (liste->coef == 0) {
            /* Le maillon de tete devient inutile : on le saute.
               Le maillon m est aussi devenu inutile (orphelin). */
            return liste->suivant;
        }
        return liste;
    }
    /* liste->expo > m->expo : on descend dans la queue. */
    liste->suivant = insererMaillon(liste->suivant, m);
    return liste;
}

/* ------------------------------------------------------------------
   Outils de lecture (identiques a la Q1).
   ------------------------------------------------------------------ */

static int car;

static void lire(void) {
    do {
        car = getchar();
    } while (car == ' ' || car == '\t');
}

static void erreur(const char *msg) {
    fprintf(stderr, "Erreur de syntaxe : %s\n", msg);
    exit(EXIT_FAILURE);
}

/* ------------------------------------------------------------------
   Analyseur a descente recursive : chaque fonction reconnait son
   non-terminal ET renvoie la valeur reconnue (un nombre, un
   exposant, ou un maillon / un debut de liste).
   ------------------------------------------------------------------ */

static POINTEUR polynome(void);
static POINTEUR monome(void);
static int      xpuissance(void);
static double   nombre(void);
static int      naturel(void);

/* naturel -> chiffre {chiffre}
   Lit un entier non signe et renvoie sa valeur numerique.
   Pas d'espace autorise entre les chiffres. */
static int naturel(void) {
    if (!isdigit(car))
        erreur("chiffre attendu");
    int n = 0;
    do {
        n = n * 10 + (car - '0');
        car = getchar();
    } while (isdigit(car));
    /* On positionne 'car' sur le prochain caractere significatif. */
    while (car == ' ' || car == '\t')
        car = getchar();
    return n;
}

/* nombre -> naturel [.{chiffre}]
   Lit un nombre eventuellement decimal et renvoie sa valeur en
   double. */
static double nombre(void) {
    double valeur = (double) naturel();
    if (car == '.') {
        /* Lecture brute pour interdire un espace apres le point. */
        car = getchar();
        double facteur = 0.1;
        while (isdigit(car)) {
            valeur += (car - '0') * facteur;
            facteur /= 10.0;
            car = getchar();
        }
        while (car == ' ' || car == '\t')
            car = getchar();
    }
    return valeur;
}

/* xpuissance -> X | X^naturel
   Lit la lettre X (suivie ou non de ^naturel) et renvoie l'exposant.
   Cas "X" tout seul : exposant 1. */
static int xpuissance(void) {
    if (car != 'X')
        erreur("'X' attendu");
    lire();
    if (car == '^') {
        lire();
        return naturel();
    }
    return 1;
}

/* monome -> nombre * xpuissance | xpuissance | nombre
   Reconnait un monome et renvoie un nouveau maillon (coef, expo).
   Le coefficient est toujours POSITIF a ce stade : la gestion du
   signe est faite par polynome() qui l'applique apres coup. */
static POINTEUR monome(void) {
    if (car == 'X') {
        /* Forme : xpuissance seule -> coefficient implicite 1. */
        int e = xpuissance();
        return nouveauMaillon(1.0, e, NULL);
    }
    if (isdigit(car)) {
        double n = nombre();
        if (car == '*') {
            /* Forme : nombre * xpuissance. */
            lire();
            int e = xpuissance();
            return nouveauMaillon(n, e, NULL);
        }
        /* Forme : nombre tout seul -> exposant 0 (constante). */
        return nouveauMaillon(n, 0, NULL);
    }
    erreur("debut de monome attendu (X ou chiffre)");
    return NULL; /* inatteignable, juste pour faire taire le compilateur */
}

/* polynome -> [-] monome { (+|-) monome }
   Construit la liste chainee triee par exposants decroissants.
   Chaque monome reconnu est insere a sa place via insererMaillon ;
   les monomes de meme exposant sont fusionnes au passage. */
static POINTEUR polynome(void) {
    int signe = 1;
    if (car == '-') {
        signe = -1;
        lire();
    }
    POINTEUR p = monome();
    p->coef *= signe;
    POINTEUR liste = insererMaillon(NULL, p);

    while (car == '+' || car == '-') {
        int s = (car == '-') ? -1 : 1;
        lire();
        POINTEUR m = monome();
        m->coef *= s;
        liste = insererMaillon(liste, m);
    }
    return liste;
}

/* ------------------------------------------------------------------
   Affichage d'un polynome (Q3).

   Parcourt la liste chainee et imprime chaque monome en respectant
   les regles d'affichage decrites en haut du fichier.
   ------------------------------------------------------------------ */
static void affiche(POINTEUR p) {
    /* Cas particulier : polynome nul represente par la liste vide. */
    if (p == NULL) {
        printf("0");
        return;
    }

    int premier = 1; /* indique si on est sur le tout premier monome */
    while (p != NULL) {
        double c = p->coef;
        int    e = p->expo;

        /* Etape 1 : afficher le separateur (signe ou rien). */
        if (premier) {
            /* Premier monome : on prefixe par '-' uniquement si
               le coefficient est negatif. Pas de '+' explicite. */
            if (c < 0)
                printf("-");
            premier = 0;
        } else {
            /* Monomes suivants : " + " ou " - " selon le signe. */
            if (c < 0)
                printf(" - ");
            else
                printf(" + ");
        }

        /* Etape 2 : afficher le coefficient (en valeur absolue, le
           signe a deja ete imprime ci-dessus). */
        double abs_c = (c < 0) ? -c : c;

        if (e == 0) {
            /* Pas de X : on ecrit toujours le coefficient. */
            printf("%g", abs_c);
        } else {
            /* On a une partie en X. On omet le coefficient s'il
               vaut 1 (ex : "X^3" au lieu de "1*X^3"). */
            if (abs_c != 1.0)
                printf("%g*", abs_c);
            /* On omet l'exposant s'il vaut 1 (ex : "X" au lieu
               de "X^1"). */
            if (e == 1)
                printf("X");
            else
                printf("X^%d", e);
        }

        p = p->suivant;
    }
}

/* ------------------------------------------------------------------
   Evaluation d'un polynome en un point (Q5).
   ------------------------------------------------------------------ */

/* Calcule x^n avec n >= 0 (cas de la grammaire : exposant naturel).
   On evite l'inclusion de math.h en faisant n multiplications. */
static double puissance(double x, int n) {
    double r = 1.0;
    for (int i = 0; i < n; i++)
        r *= x;
    return r;
}

/* Calcule et renvoie la valeur du polynome p pour la variable X = x.
   Le polynome nul (liste vide) vaut 0 par convention. */
static double eval(POINTEUR p, double x) {
    double total = 0.0;
    while (p != NULL) {
        total += p->coef * puissance(x, p->expo);
        p = p->suivant;
    }
    return total;
}

int main(int argc, char **argv) {
    lire();
    POINTEUR p = polynome();
    if (car != '\n' && car != EOF)
        erreur("caractere inattendu apres le polynome");
    printf("polynome reconnu : ");
    affiche(p);
    printf("\n");

    /* Si l'utilisateur a fourni une valeur de x en argument, on
       calcule P(x) et on l'affiche. */
    if (argc >= 2) {
        double x = atof(argv[1]);
        printf("P(%g) = %g\n", x, eval(p, x));
    }
    return 0;
}
