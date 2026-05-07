/*
 * Question 7 (Java) : Recyclage de la memoire (garbage collector).
 *
 * Reprend la Q6 et y AJOUTE un mecanisme de recyclage de la memoire
 * selon l'algorithme classique "mark and sweep". Comme en C, le GC
 * travaille "en sous-sol" : AUCUNE des methodes de la Q6 (plus,
 * moins, fois, quotient, copie, neg, ...) n'a ete modifiee.
 *
 * Particularite Java importante : Java possede deja un garbage
 * collector integre, donc en pratique ce mecanisme n'a pas d'utilite
 * concrete (les maillons orphelins sont liberes automatiquement par
 * la JVM des qu'ils deviennent inaccessibles). On l'implemente quand
 * meme parce que :
 *   1) Le sujet l'exige.
 *   2) C'est une excellente illustration de l'algorithme mark-and-
 *      sweep, qui est exactement celui utilise par certaines JVMs.
 *   3) Cela montre qu'on comprend le mecanisme, qui reste utile dans
 *      des langages sans GC (C, C++ avant les smart pointers).
 *
 * Notre simulation suit la meme structure qu'en C :
 *   - chaque Maillon a deux champs supplementaires : 'general'
 *     (chainage de tous les maillons) et 'utile' (drapeau de
 *     marquage).
 *   - tousLesMaillons : tete de la grande liste de tous les maillons.
 *   - polyUtile[]     : tableau des polynomes utiles (racines).
 *   - recycler()      : marque puis "balaie".
 *
 * Difference avec le C : au lieu d'appeler free(), recycler() retire
 * simplement les maillons de tousLesMaillons. Comme plus aucune
 * reference ne pointe vers eux (ils n'etaient pas marques donc pas
 * dans polyUtile), le GC integre de Java les recuperera tot ou tard.
 *
 * Le seul changement par rapport a la Q6 est dans le CONSTRUCTEUR
 * de Maillon : il ajoute automatiquement chaque nouveau maillon a
 * tousLesMaillons. Toute la Q6 beneficie ainsi du mecanisme sans
 * modification de code.
 *
 * --- Q6 ---
 *
 * Reprend la Q5 et AJOUTE les quatre operations imposees :
 *   static Maillon            plus    (Maillon a, Maillon b);
 *   static Maillon            moins   (Maillon a, Maillon b);
 *   static Maillon            fois    (Maillon a, Maillon b);
 *   static DivisionResultat   quotient(Maillon a, Maillon b);
 *
 * Regles d'or imposees par le sujet :
 *   1) Ne JAMAIS modifier les polynomes a et b passes en parametre.
 *      Le resultat est toujours une nouvelle liste, allouee depuis
 *      zero. On prefere allouer un maillon plutot que reutiliser un
 *      maillon existant.
 *   2) On ne libere RIEN. (En Java, c'est le GC integre qui s'en
 *      chargera automatiquement, sans aucun code de notre part.)
 *
 * Particularite Java : le quotient renvoie a la fois un quotient ET
 * un reste. En C on utilisait un pointeur sur pointeur (POINTEUR *).
 * En Java, on cree une classe 'DivisionResultat' qui regroupe les
 * deux valeurs et on la renvoie en bloc. C'est plus propre que les
 * "out parameters" du C.
 *
 * Strategie d'implementation (identique a la Q6 C) :
 *   - plus : fusion (merge) des deux listes triees.
 *   - moins : reutilise plus, via un polynome auxiliaire neg(b).
 *   - fois : formule recursive du sujet
 *       P*Q = a*b * X^(n+m) + a*X^n * Q' + P' * Q
 *   - quotient : division euclidienne posee.
 *
 * Petite amelioration sur insererMaillon : on rejette desormais les
 * coefficients nuls. Ainsi le polynome nul est TOUJOURS represente
 * par null, meme si l'utilisateur tape "0".
 *
 * Le programme lit DEUX polynomes (un par ligne) et affiche le
 * resultat des quatre operations.
 */

import java.io.IOException;

public class Analyseur {

    /* ------------------------------------------------------------------
       Structure de donnees : Maillon (equivalent de la struct C).
       ------------------------------------------------------------------ */

    static class Maillon {
        double  coef;
        int     expo;
        Maillon suivant;
        /* Champs ajoutes pour le GC (Q7) :
             general : chainage de TOUS les maillons existants.
             utile   : drapeau utilise par la phase de marquage. */
        Maillon general;
        boolean utile;

        Maillon(double coef, int expo, Maillon suivant) {
            this.coef = coef;
            this.expo = expo;
            this.suivant = suivant;
            this.utile = false;
            /* Modification Q7 : chaque nouveau maillon est ajoute en
               tete de la liste globale 'tousLesMaillons'. C'est
               l'UNIQUE modification necessaire pour que la Q6
               beneficie du GC sans changer aucune autre ligne. */
            this.general = tousLesMaillons;
            tousLesMaillons = this;
            nbMaillonsTraques++;
        }
    }

    /* Tete de la grande liste de tous les maillons traques par le GC.
       Mise a jour par le constructeur de Maillon (ajout en tete) et
       par recycler() (filtrage). */
    private static Maillon tousLesMaillons = null;

    /* Compteur du nombre de maillons actuellement traques (utilise
       uniquement pour la demonstration pedagogique). */
    private static int nbMaillonsTraques = 0;

    /* Insere le maillon m dans la liste triee 'liste' (degres
       decroissants) et renvoie la nouvelle tete de la liste.

       Trois cas selon l'exposant de m compare a celui du premier
       maillon :
         - liste vide ou expo de m strictement plus grand : m devient
           la nouvelle tete.
         - meme exposant : on FUSIONNE en additionnant les coefficients
           sur le maillon existant ; si le total devient nul, on
           retire ce maillon.
         - expo de m strictement plus petit : on descend dans la
           queue par appel recursif.

       Le maillon m peut devenir orphelin si la fusion l'absorbe ;
       le garbage collector de Java s'en chargera automatiquement. */
    private static Maillon insererMaillon(Maillon liste, Maillon m) {
        /* Un monome de coefficient nul n'apporte rien : on le rejette
           directement (m devient orphelin, le GC Java s'en occupera). */
        if (m.coef == 0)
            return liste;
        if (liste == null || liste.expo < m.expo) {
            m.suivant = liste;
            return m;
        }
        if (liste.expo == m.expo) {
            liste.coef += m.coef;
            if (liste.coef == 0) {
                /* Le maillon de tete devient inutile : on le saute. */
                return liste.suivant;
            }
            return liste;
        }
        /* liste.expo > m.expo : on descend dans la queue. */
        liste.suivant = insererMaillon(liste.suivant, m);
        return liste;
    }

    /* ------------------------------------------------------------------
       Outils de lecture (identiques a la Q1).
       ------------------------------------------------------------------ */

    private static int car;

    private static void lire() throws IOException {
        do {
            car = System.in.read();
        } while (car == ' ' || car == '\t');
    }

    private static void erreur(String msg) {
        System.err.println("Erreur de syntaxe : " + msg);
        System.exit(1);
    }

    /* ------------------------------------------------------------------
       Analyseur a descente recursive : chaque fonction reconnait son
       non-terminal ET renvoie la valeur reconnue.
       ------------------------------------------------------------------ */

    /* naturel -> chiffre {chiffre}
       Lit un entier non signe et renvoie sa valeur. */
    private static int naturel() throws IOException {
        if (!Character.isDigit(car))
            erreur("chiffre attendu");
        int n = 0;
        do {
            n = n * 10 + (car - '0');
            car = System.in.read();
        } while (Character.isDigit(car));
        while (car == ' ' || car == '\t')
            car = System.in.read();
        return n;
    }

    /* nombre -> naturel [.{chiffre}]
       Lit un nombre eventuellement decimal et renvoie sa valeur. */
    private static double nombre() throws IOException {
        double valeur = naturel();
        if (car == '.') {
            car = System.in.read();
            double facteur = 0.1;
            while (Character.isDigit(car)) {
                valeur += (car - '0') * facteur;
                facteur /= 10.0;
                car = System.in.read();
            }
            while (car == ' ' || car == '\t')
                car = System.in.read();
        }
        return valeur;
    }

    /* xpuissance -> X | X^naturel
       Renvoie l'exposant (1 si X tout seul). */
    private static int xpuissance() throws IOException {
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
       Renvoie un nouveau maillon (coef, expo). Le coefficient est
       toujours positif a ce stade ; la gestion du signe est faite
       par polynome(). */
    private static Maillon monome() throws IOException {
        if (car == 'X') {
            int e = xpuissance();
            return new Maillon(1.0, e, null);
        }
        if (Character.isDigit(car)) {
            double n = nombre();
            if (car == '*') {
                lire();
                int e = xpuissance();
                return new Maillon(n, e, null);
            }
            return new Maillon(n, 0, null);
        }
        erreur("debut de monome attendu (X ou chiffre)");
        return null; /* inatteignable, mais Java exige un return */
    }

    /* polynome -> [-] monome { (+|-) monome }
       Construit la liste chainee triee par exposants decroissants.
       Chaque monome reconnu est insere a sa place via insererMaillon ;
       les monomes de meme exposant sont fusionnes au passage. */
    private static Maillon polynome() throws IOException {
        int signe = 1;
        if (car == '-') {
            signe = -1;
            lire();
        }
        Maillon p = monome();
        p.coef *= signe;
        Maillon liste = insererMaillon(null, p);

        while (car == '+' || car == '-') {
            int s = (car == '-') ? -1 : 1;
            lire();
            Maillon m = monome();
            m.coef *= s;
            liste = insererMaillon(liste, m);
        }
        return liste;
    }

    /* ------------------------------------------------------------------
       Affichage d'un polynome (Q3).
       ------------------------------------------------------------------ */

    /* Petit utilitaire : formate un double comme C avec %g, c'est-a-dire
       sans le ".0" superflu pour les valeurs entieres (5.0 -> "5"). */
    private static String fmt(double d) {
        if (d == (long) d)
            return String.valueOf((long) d);
        return String.valueOf(d);
    }

    /* Imprime le polynome p sur la sortie standard. Suit les regles
       d'affichage decrites en haut du fichier. */
    private static void affiche(Maillon p) {
        /* Cas particulier : polynome nul represente par la liste vide. */
        if (p == null) {
            System.out.print("0");
            return;
        }

        StringBuilder sb = new StringBuilder();
        boolean premier = true;
        while (p != null) {
            double c = p.coef;
            int    e = p.expo;

            /* Etape 1 : afficher le separateur (signe ou rien). */
            if (premier) {
                if (c < 0)
                    sb.append("-");
                premier = false;
            } else {
                if (c < 0)
                    sb.append(" - ");
                else
                    sb.append(" + ");
            }

            /* Etape 2 : afficher le coefficient (en valeur absolue). */
            double absC = (c < 0) ? -c : c;

            if (e == 0) {
                sb.append(fmt(absC));
            } else {
                if (absC != 1.0)
                    sb.append(fmt(absC)).append("*");
                if (e == 1)
                    sb.append("X");
                else
                    sb.append("X^").append(e);
            }
            p = p.suivant;
        }
        System.out.print(sb.toString());
    }

    /* ------------------------------------------------------------------
       Evaluation d'un polynome en un point (Q5).
       ------------------------------------------------------------------ */

    /* Calcule x^n avec n >= 0 (cas de la grammaire : exposant naturel).
       On evite d'utiliser Math.pow en faisant n multiplications, ce
       qui donne un resultat exact pour des exposants raisonnables. */
    private static double puissance(double x, int n) {
        double r = 1.0;
        for (int i = 0; i < n; i++)
            r *= x;
        return r;
    }

    /* Calcule et renvoie la valeur du polynome p pour la variable
       X = x. Le polynome nul (liste vide) vaut 0 par convention. */
    private static double eval(Maillon p, double x) {
        double total = 0.0;
        while (p != null) {
            total += p.coef * puissance(x, p.expo);
            p = p.suivant;
        }
        return total;
    }

    /* ==================================================================
       Operations arithmetiques (Q6)
       ==================================================================
       Toutes ces methodes respectent les regles d'or :
         - elles ne modifient jamais leurs parametres a et b ;
         - elles allouent librement des maillons neufs et n'en liberent
           aucun. Le GC de Java se charge automatiquement de recuperer
           les maillons orphelins.
       ================================================================== */

    /* Classe resultat pour la division euclidienne : regroupe le
       quotient et le reste. C'est l'equivalent Java propre du
       "POINTEUR *reste" du C. */
    static class DivisionResultat {
        Maillon quotient;
        Maillon reste;

        DivisionResultat(Maillon quotient, Maillon reste) {
            this.quotient = quotient;
            this.reste = reste;
        }
    }

    /* Renvoie une COPIE profonde de p (chaque maillon est reduplique). */
    private static Maillon copie(Maillon p) {
        if (p == null)
            return null;
        return new Maillon(p.coef, p.expo, copie(p.suivant));
    }

    /* Renvoie un nouveau polynome egal a -p (chaque coefficient est
       negre ; les exposants restent inchanges). */
    private static Maillon neg(Maillon p) {
        if (p == null)
            return null;
        return new Maillon(-p.coef, p.expo, neg(p.suivant));
    }

    /* Resultat partage de la fusion : on construit la liste en queue.
       En Java, comme on n'a pas de pointeur sur pointeur, on utilise
       une petite classe temporaire pour suivre la tete et la queue. */
    private static class Constructeur {
        Maillon tete = null;
        Maillon queue = null;

        void ajouter(double coef, int expo) {
            if (coef == 0) return; /* on ne propage pas les coefs nuls */
            Maillon m = new Maillon(coef, expo, null);
            if (tete == null) {
                tete = m;
            } else {
                queue.suivant = m;
            }
            queue = m;
        }
    }

    /* Addition de deux polynomes : algorithme de fusion (merge) sur
       deux listes triees par exposants decroissants. */
    private static Maillon plus(Maillon a, Maillon b) {
        Constructeur c = new Constructeur();

        while (a != null && b != null) {
            if (a.expo > b.expo) {
                c.ajouter(a.coef, a.expo);
                a = a.suivant;
            } else if (a.expo < b.expo) {
                c.ajouter(b.coef, b.expo);
                b = b.suivant;
            } else {
                /* Exposants egaux : on additionne. Si la somme est nulle,
                   ajouter() ne creera rien. */
                c.ajouter(a.coef + b.coef, a.expo);
                a = a.suivant;
                b = b.suivant;
            }
        }
        /* On copie les maillons restants de la liste non epuisee. */
        while (a != null) { c.ajouter(a.coef, a.expo); a = a.suivant; }
        while (b != null) { c.ajouter(b.coef, b.expo); b = b.suivant; }
        return c.tete;
    }

    /* Soustraction : reutilise plus avec b nie. */
    private static Maillon moins(Maillon a, Maillon b) {
        return plus(a, neg(b));
    }

    /* Multiplication, formule recursive du sujet :
         P*Q = a_n*b_m * X^(n+m)
             + (a_n * X^n) * Q'
             + P' * Q
       Cas de base : si a ou b est nul, le produit est nul. */
    private static Maillon fois(Maillon a, Maillon b) {
        if (a == null || b == null)
            return null;

        /* Polynome a un seul monome egal a la tete de a. */
        Maillon teteA = new Maillon(a.coef, a.expo, null);

        /* Morceau 1 : produit des deux tetes (un seul monome). */
        Maillon partie1 = new Maillon(a.coef * b.coef,
                                      a.expo + b.expo, null);

        /* Morceau 2 : tete de a multipliee par la queue de b. */
        Maillon partie2 = fois(teteA, b.suivant);

        /* Morceau 3 : queue de a multipliee par tout b. */
        Maillon partie3 = fois(a.suivant, b);

        /* Somme des trois morceaux. */
        return plus(plus(partie1, partie2), partie3);
    }

    /* Division euclidienne. Renvoie un objet contenant le quotient et
       le reste. Si b est nul, on signale une erreur. */
    private static DivisionResultat quotient(Maillon a, Maillon b) {
        if (b == null) {
            System.err.println("Erreur : division par le polynome nul");
            System.exit(1);
        }

        Maillon q = null;          /* le quotient construit petit a petit */
        Maillon r = copie(a);      /* le reste, demarre comme une copie de a */

        while (r != null && r.expo >= b.expo) {
            /* Monome t tel que t * tete(b) = tete(r). */
            double tCoef = r.coef / b.coef;
            int    tExpo = r.expo - b.expo;
            Maillon t = new Maillon(tCoef, tExpo, null);

            q = plus(q, t);
            r = moins(r, fois(t, b));
        }
        return new DivisionResultat(q, r);
    }

    /* ==================================================================
       Garbage collector (Q7)
       ================================================================== */

    /* Tableau global des polynomes utiles. C'est l'utilisateur qui y
       inscrit les polynomes a preserver lors du recyclage. */
    private static final int MAX_POLY_UTILES = 100;
    private static Maillon[] polyUtile = new Maillon[MAX_POLY_UTILES];
    private static int       nbPolyUtile = 0;

    /* Inscrit p dans le tableau des polynomes utiles. */
    private static void enregistrer(Maillon p) {
        if (nbPolyUtile >= MAX_POLY_UTILES) {
            System.err.println("Erreur : tableau polyUtile sature");
            System.exit(1);
        }
        polyUtile[nbPolyUtile++] = p;
    }

    /* Recycle la memoire selon l'algorithme mark & sweep.

       Phase 1 (marquage) : on parcourt chaque polynome utile via
       'suivant' et on pose le drapeau utile = true sur tous ses
       maillons.

       Phase 2 (balayage) : on parcourt la grande liste tousLesMaillons
       via 'general'. Pour chaque maillon :
         - s'il est marque, on le conserve (et on efface son drapeau
           pour le prochain recyclage), en le rechainant dans une
           nouvelle liste 'nouvelleTete' ;
         - sinon, on le retire de tousLesMaillons. En Java il n'y a
           pas de free() : il suffit qu'aucune reference ne pointe
           plus vers le maillon pour que le GC integre le recupere.
           Ici, comme le maillon n'etait pas marque, c'est qu'il
           n'appartenait a aucun polynome utile, donc son retrait
           de tousLesMaillons supprime sa derniere reference connue.
    */
    private static void recycler() {
        /* Phase 1 : marquage. */
        for (int i = 0; i < nbPolyUtile; i++) {
            Maillon m = polyUtile[i];
            while (m != null) {
                m.utile = true;
                m = m.suivant;
            }
        }

        /* Phase 2 : balayage. */
        Maillon nouvelleTete = null;
        Maillon m = tousLesMaillons;
        while (m != null) {
            /* On sauvegarde le pointeur 'general' avant de pouvoir
               eventuellement le perdre. */
            Maillon suiv = m.general;
            if (m.utile) {
                /* On garde ce maillon : on efface le drapeau et on
                   le rechaine dans la nouvelle liste globale. */
                m.utile = false;
                m.general = nouvelleTete;
                nouvelleTete = m;
            } else {
                /* On detache le maillon : son lien 'general' est
                   coupe, et il n'est plus referencé. Le GC de Java
                   le recuperera automatiquement. */
                m.general = null;
                nbMaillonsTraques--;
            }
            m = suiv;
        }
        tousLesMaillons = nouvelleTete;
    }

    /* ==================================================================
       Programme principal : lit deux polynomes, fait les operations,
       puis demontre le mecanisme de recyclage.
       ================================================================== */

    public static void main(String[] args) throws IOException {
        /* Premiere ligne : polynome A. */
        lire();
        Maillon a = polynome();
        if (car != '\n' && car != -1)
            erreur("caractere inattendu apres le premier polynome");

        /* Deuxieme ligne : polynome B. */
        lire();
        Maillon b = polynome();
        if (car != '\n' && car != -1)
            erreur("caractere inattendu apres le second polynome");

        /* On enregistre A et B comme utiles : ce sont les "racines"
           que le recyclage doit preserver. */
        enregistrer(a);
        enregistrer(b);

        System.out.print("A = ");      affiche(a); System.out.println();
        System.out.print("B = ");      affiche(b); System.out.println();
        System.out.print("A + B = ");  affiche(plus(a, b));  System.out.println();
        System.out.print("A - B = ");  affiche(moins(a, b)); System.out.println();
        System.out.print("A * B = ");  affiche(fois(a, b));  System.out.println();

        if (b == null) {
            System.out.println("A / B impossible (B est le polynome nul)");
        } else {
            DivisionResultat dr = quotient(a, b);
            System.out.print("A / B = quotient ");
            affiche(dr.quotient);
            System.out.print(" , reste ");
            affiche(dr.reste);
            System.out.println();
        }

        /* Demonstration du GC : avant recyclage, on a alloue plein de
           maillons. Apres recyclage, seuls les maillons appartenant
           a A ou B doivent rester traques. */
        int avant = nbMaillonsTraques;
        recycler();
        int apres = nbMaillonsTraques;

        System.out.println();
        System.out.println("--- Garbage collector ---");
        System.out.println("Maillons traques avant recyclage : " + avant);
        System.out.println("Maillons traques apres recyclage : " + apres);
        System.out.println("Maillons detaches                : " + (avant - apres));

        /* Verification : A et B doivent toujours etre intacts. */
        System.out.print("Apres recyclage, A = ");
        affiche(a);
        System.out.println();
        System.out.print("Apres recyclage, B = ");
        affiche(b);
        System.out.println();
    }
}
