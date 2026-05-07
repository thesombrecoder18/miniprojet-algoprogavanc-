/*
 * Question 7 : Recyclage de la memoire (garbage collector).
 *
 * Ce fichier reprend la Q6 et y AJOUTE un mecanisme de recyclage de
 * la memoire (garbage collector, ou GC) selon l'algorithme classique
 * "mark and sweep". Conformement au sujet, le GC travaille "en
 * sous-sol" : AUCUNE des fonctions de la Q6 (plus, moins, fois,
 * quotient, copie, neg, polynome, monome, ...) n'a ete modifiee.
 *
 * Tout repose sur le fait que la Q6 alloue uniquement via la fonction
 * nouveauMaillon() : il suffit d'enrichir cette unique fonction pour
 * que tous les maillons rejoignent automatiquement la grande liste
 * 'tousLesMaillons' utilisee par le GC.
 *
 * Ajouts par rapport a la Q6 :
 *   - struct maillon : deux nouveaux champs 'general' (chainage de
 *     tous les maillons) et 'utile' (drapeau de marquage).
 *   - tousLesMaillons : tete de la grande liste de tous les maillons.
 *   - polyUtile[]     : tableau des polynomes que l'utilisateur juge
 *     encore utiles (les "racines" pour le marquage).
 *   - enregistrer(p)  : ajoute p au tableau polyUtile.
 *   - recycler()      : execute la phase de marquage suivie de la
 *     phase de balayage (libere les maillons non marques).
 *
 * Algorithme du GC :
 *   Phase 1 (mark) : pour chaque polynome de polyUtile, on parcourt
 *   ses maillons via 'suivant' et on met utile=1.
 *   Phase 2 (sweep) : on parcourt tousLesMaillons via 'general'.
 *   Les maillons marques sont reinseres dans la nouvelle liste
 *   globale (et leur drapeau remis a 0). Les non marques sont
 *   liberes par free().
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
    /* Champs ajoutes pour le garbage collector (Q7) :
       - general : chainage de TOUS les maillons existants (independant
         du chainage 'suivant' qui structure les polynomes).
       - utile   : drapeau utilise par la phase de marquage. */
    struct maillon *general;
    int             utile;
} MAILLON;

typedef MAILLON *POINTEUR;

/* Tete de la grande liste de tous les maillons jamais alloues et
   pas encore liberes. Mise a jour par nouveauMaillon (ajout en
   tete) et par recycler (filtrage). */
static POINTEUR tousLesMaillons = NULL;

/* Compteur du nombre total de maillons actuellement vivants
   (utilise uniquement pour les statistiques pedagogiques). */
static int nbMaillonsVivants = 0;

/* Alloue un nouveau maillon avec les valeurs donnees. En cas d'echec
   d'allocation, on arrete le programme.
   Modification Q7 : chaque maillon alloue est ajoute en tete de
   la liste globale 'tousLesMaillons' via le champ 'general'. C'est
   l'UNIQUE modification necessaire pour que la Q6 beneficie du GC
   sans changer aucune autre ligne. */
static POINTEUR nouveauMaillon(double coef, int expo, POINTEUR suivant) {
    POINTEUR m = (POINTEUR) malloc(sizeof(MAILLON));
    if (m == NULL) {
        fprintf(stderr, "Erreur : memoire insuffisante\n");
        exit(EXIT_FAILURE);
    }
    m->coef = coef;
    m->expo = expo;
    m->suivant = suivant;
    m->utile = 0;
    /* Insertion en tete de la grande liste globale. */
    m->general = tousLesMaillons;
    tousLesMaillons = m;
    nbMaillonsVivants++;
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
    /* Un monome de coefficient nul n'apporte rien : on le rejette
       directement (m devient orphelin, c'est sans importance). */
    if (m->coef == 0)
        return liste;
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
   On evite l'inclusion de math.h en faisant n multiplications.
   __attribute__((unused)) : eval n'est plus appelee dans le main de
   la Q6 mais on la conserve, conformement a la consigne "rien de ce
   qui constituait la Q5 ne doit disparaitre". */
__attribute__((unused))
static double puissance(double x, int n) {
    double r = 1.0;
    for (int i = 0; i < n; i++)
        r *= x;
    return r;
}

/* Calcule et renvoie la valeur du polynome p pour la variable X = x.
   Le polynome nul (liste vide) vaut 0 par convention. */
__attribute__((unused))
static double eval(POINTEUR p, double x) {
    double total = 0.0;
    while (p != NULL) {
        total += p->coef * puissance(x, p->expo);
        p = p->suivant;
    }
    return total;
}

/* ==================================================================
   Operations arithmetiques (Q6)
   ==================================================================
   Toutes ces fonctions respectent les regles d'or :
     - elles ne modifient jamais leurs parametres a et b ;
     - elles allouent librement des maillons neufs et n'en liberent
       aucun. Les maillons orphelins seront recycles a la Q7.
   ================================================================== */

/* Renvoie une COPIE profonde de p (chaque maillon est reduplique).
   Si p est NULL, renvoie NULL. */
static POINTEUR copie(POINTEUR p) {
    if (p == NULL)
        return NULL;
    return nouveauMaillon(p->coef, p->expo, copie(p->suivant));
}

/* Renvoie un nouveau polynome egal a -p (chaque coefficient est
   negrie ; les exposants restent inchanges). */
static POINTEUR neg(POINTEUR p) {
    if (p == NULL)
        return NULL;
    return nouveauMaillon(-p->coef, p->expo, neg(p->suivant));
}

/* Petit utilitaire : ajoute le maillon (coef, expo) en queue de la
   liste chainee dont 'tete' est la tete et 'queue' le dernier
   maillon. Met a jour 'queue' et 'tete' si necessaire.
   On ajoute en queue pour preserver l'ordre des exposants
   decroissants produit par la fusion. */
static void ajouterEnQueue(POINTEUR *tete, POINTEUR *queue,
                           double coef, int expo) {
    if (coef == 0)
        return; /* on ne propage pas les coefs nuls */
    POINTEUR m = nouveauMaillon(coef, expo, NULL);
    if (*tete == NULL)
        *tete = m;
    else
        (*queue)->suivant = m;
    *queue = m;
}

/* Addition de deux polynomes : algorithme de fusion (merge) sur
   deux listes triees par exposants decroissants. */
static POINTEUR plus(POINTEUR a, POINTEUR b) {
    POINTEUR tete = NULL, queue = NULL;

    while (a != NULL && b != NULL) {
        if (a->expo > b->expo) {
            ajouterEnQueue(&tete, &queue, a->coef, a->expo);
            a = a->suivant;
        } else if (a->expo < b->expo) {
            ajouterEnQueue(&tete, &queue, b->coef, b->expo);
            b = b->suivant;
        } else {
            /* Exposants egaux : on additionne. Si la somme est nulle,
               ajouterEnQueue ne creera rien. */
            ajouterEnQueue(&tete, &queue, a->coef + b->coef, a->expo);
            a = a->suivant;
            b = b->suivant;
        }
    }
    /* On copie les maillons restants de la liste non epuisee. */
    while (a != NULL) {
        ajouterEnQueue(&tete, &queue, a->coef, a->expo);
        a = a->suivant;
    }
    while (b != NULL) {
        ajouterEnQueue(&tete, &queue, b->coef, b->expo);
        b = b->suivant;
    }
    return tete;
}

/* Soustraction : reutilise plus avec b nie. */
static POINTEUR moins(POINTEUR a, POINTEUR b) {
    return plus(a, neg(b));
}

/* Multiplication, formule recursive du sujet :
     P*Q = a_n*b_m * X^(n+m)
         + (a_n * X^n) * Q'
         + P' * Q
   ou a_n*X^n est la tete de P et b_m*X^m la tete de Q.

   Cas de base : si a ou b est nul, le produit est nul.
   Sinon on fabrique les trois morceaux et on les somme avec plus. */
static POINTEUR fois(POINTEUR a, POINTEUR b) {
    if (a == NULL || b == NULL)
        return NULL;

    /* Polynome a un seul monome egal a la tete de a (a_n * X^n). */
    POINTEUR teteA = nouveauMaillon(a->coef, a->expo, NULL);

    /* Morceau 1 : produit des deux tetes (un seul monome). */
    POINTEUR partie1 = nouveauMaillon(a->coef * b->coef,
                                      a->expo + b->expo, NULL);

    /* Morceau 2 : tete de a multipliee par la queue de b. */
    POINTEUR partie2 = fois(teteA, b->suivant);

    /* Morceau 3 : queue de a multipliee par tout b. */
    POINTEUR partie3 = fois(a->suivant, b);

    /* Somme des trois morceaux. */
    return plus(plus(partie1, partie2), partie3);
}

/* Division euclidienne : renvoie le quotient et range le reste a
   l'adresse pointee par 'reste'.
   Algorithme classique de la division posee :
     q = 0
     r = a
     tant que degre(r) >= degre(b) :
       t = monome tel que t * tete(b) = tete(r)
       q = q + t
       r = r - t * b
   Si b est nul, on signale une erreur (division par zero). */
static POINTEUR quotient(POINTEUR a, POINTEUR b, POINTEUR *reste) {
    if (b == NULL) {
        fprintf(stderr, "Erreur : division par le polynome nul\n");
        exit(EXIT_FAILURE);
    }

    POINTEUR q = NULL;       /* le quotient construit petit a petit */
    POINTEUR r = copie(a);   /* le reste, demarre comme une copie de a */

    /* Tant que le degre du reste est >= au degre de b, on peut
       continuer la division. */
    while (r != NULL && r->expo >= b->expo) {
        /* On calcule le monome t tel que t * tete(b) = tete(r). */
        double tCoef = r->coef / b->coef;
        int    tExpo = r->expo - b->expo;
        POINTEUR t = nouveauMaillon(tCoef, tExpo, NULL);

        /* On l'ajoute au quotient et on retire t*b du reste. */
        q = plus(q, t);
        r = moins(r, fois(t, b));
    }

    *reste = r;
    return q;
}

/* ==================================================================
   Garbage collector (Q7)
   ================================================================== */

/* Tableau global des polynomes utiles. C'est l'utilisateur qui y
   inscrit les polynomes a preserver lors du recyclage. */
#define MAX_POLY_UTILES 100
static POINTEUR polyUtile[MAX_POLY_UTILES];
static int      nbPolyUtile = 0;

/* Inscrit p dans le tableau des polynomes utiles. */
static void enregistrer(POINTEUR p) {
    if (nbPolyUtile >= MAX_POLY_UTILES) {
        fprintf(stderr, "Erreur : tableau polyUtile sature\n");
        exit(EXIT_FAILURE);
    }
    polyUtile[nbPolyUtile++] = p;
}

/* Vide le tableau des polynomes utiles (utile pour les tests : avant
   un recyclage, on peut tout desinscrire sauf ce qu'on veut garder). */
static void videUtiles(void) {
    nbPolyUtile = 0;
}

/* Recycle la memoire selon l'algorithme mark & sweep.

   Phase 1 (marquage) : on parcourt chaque polynome utile via
   'suivant' et on pose le drapeau 'utile' sur tous ses maillons.

   Phase 2 (balayage) : on parcourt la grande liste tousLesMaillons
   via 'general'. Pour chaque maillon :
     - s'il est marque, on le conserve (et on efface son drapeau
       pour le prochain recyclage), en le rechainant dans une
       nouvelle liste 'nouvelleTete' ;
     - sinon, on le libere avec free().

   La nouvelle liste 'nouvelleTete' devient la nouvelle valeur de
   tousLesMaillons. */
static void recycler(void) {
    /* Phase 1 : marquage. */
    for (int i = 0; i < nbPolyUtile; i++) {
        POINTEUR m = polyUtile[i];
        while (m != NULL) {
            m->utile = 1;
            m = m->suivant;
        }
    }

    /* Phase 2 : balayage. On reconstruit tousLesMaillons en ne
       gardant que les maillons marques. */
    POINTEUR nouvelleTete = NULL;
    POINTEUR m = tousLesMaillons;
    while (m != NULL) {
        /* IMPORTANT : sauvegarder le pointeur 'general' AVANT le
           free(), car free(m) rend m->general indefini. */
        POINTEUR suiv = m->general;
        if (m->utile) {
            /* On garde ce maillon : on efface le drapeau et on le
               rechaine dans la nouvelle liste globale. */
            m->utile = 0;
            m->general = nouvelleTete;
            nouvelleTete = m;
        } else {
            /* On libere ce maillon. */
            free(m);
            nbMaillonsVivants--;
        }
        m = suiv;
    }
    tousLesMaillons = nouvelleTete;
}

/* ==================================================================
   Programme principal : lit deux polynomes, fait les operations,
   puis demontre le mecanisme de recyclage.
   ================================================================== */

int main(void) {
    /* Premiere ligne : polynome A. */
    lire();
    POINTEUR a = polynome();
    if (car != '\n' && car != EOF)
        erreur("caractere inattendu apres le premier polynome");

    /* Deuxieme ligne : polynome B. lire() saute le '\n' et
       positionne 'car' sur le premier caractere de la ligne 2. */
    lire();
    POINTEUR b = polynome();
    if (car != '\n' && car != EOF)
        erreur("caractere inattendu apres le second polynome");

    /* On enregistre A et B comme utiles : ce sont les "racines" que
       le recyclage doit preserver. */
    enregistrer(a);
    enregistrer(b);

    printf("A = ");      affiche(a); printf("\n");
    printf("B = ");      affiche(b); printf("\n");
    printf("A + B = ");  affiche(plus(a, b));  printf("\n");
    printf("A - B = ");  affiche(moins(a, b)); printf("\n");
    printf("A * B = ");  affiche(fois(a, b));  printf("\n");

    if (b == NULL) {
        printf("A / B impossible (B est le polynome nul)\n");
    } else {
        POINTEUR reste;
        POINTEUR q = quotient(a, b, &reste);
        printf("A / B = quotient ");
        affiche(q);
        printf(" , reste ");
        affiche(reste);
        printf("\n");
    }

    /* Demonstration du GC : avant recyclage, on a alloue plein de
       maillons (resultats des operations + intermediaires). Apres
       recyclage, seuls les maillons appartenant a A ou B doivent
       rester. */
    int avant = nbMaillonsVivants;
    recycler();
    int apres = nbMaillonsVivants;

    printf("\n");
    printf("--- Garbage collector ---\n");
    printf("Maillons vivants avant recyclage : %d\n", avant);
    printf("Maillons vivants apres recyclage : %d\n", apres);
    printf("Maillons liberes                 : %d\n", avant - apres);

    /* Verification : A et B doivent toujours etre intacts. */
    printf("Apres recyclage, A = ");
    affiche(a);
    printf("\n");
    printf("Apres recyclage, B = ");
    affiche(b);
    printf("\n");

    /* Pour eviter un avertissement sur la fonction de service. */
    (void) videUtiles;
    return 0;
}
