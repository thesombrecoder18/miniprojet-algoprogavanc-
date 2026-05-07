/*
 * Question 5 (Java) : Evaluation d'un polynome en un point.
 *
 * Reprend la Q4 et y AJOUTE une methode
 *   static double eval(Maillon p, double x);
 * qui calcule la valeur du polynome p pour la variable X = x.
 *
 * Methode : on parcourt la liste chainee et on accumule
 *   total = somme des (coef_i * x^expo_i)
 *
 * Pour pouvoir tester, le programme accepte une valeur de x en
 * argument optionnel sur la ligne de commande :
 *   java Analyseur          -> affiche juste le polynome
 *   java Analyseur 2.5      -> affiche aussi P(2.5)
 *
 * Pour un x negatif, il faut le quoter pour le shell :
 *   java Analyseur "-2"
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

        Maillon(double coef, int expo, Maillon suivant) {
            this.coef = coef;
            this.expo = expo;
            this.suivant = suivant;
        }
    }

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

    public static void main(String[] args) throws IOException {
        lire();
        Maillon p = polynome();
        if (car != '\n' && car != -1)
            erreur("caractere inattendu apres le polynome");
        System.out.print("polynome reconnu : ");
        affiche(p);
        System.out.println();

        /* Si l'utilisateur a fourni une valeur de x en argument, on
           calcule P(x) et on l'affiche. */
        if (args.length >= 1) {
            double x = Double.parseDouble(args[0]);
            double y = eval(p, x);
            System.out.println("P(" + fmt(x) + ") = " + fmt(y));
        }
    }
}
