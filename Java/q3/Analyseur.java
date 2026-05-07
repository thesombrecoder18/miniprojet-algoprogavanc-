/*
 * Question 3 (Java) : Affichage d'un polynome.
 *
 * Reprend la Q2 et y AJOUTE une fonction 'affiche' qui imprime le
 * polynome sous une forme lisible par un humain, du genre :
 *   "-4.5*X^5 + 2*X^4 + X^3 - X + 123"
 *
 * Regles d'affichage appliquees :
 *   - liste vide                  -> "0"
 *   - premier monome negatif      -> commence par "-" (sans espace)
 *   - premier monome positif      -> aucun signe
 *   - monomes suivants            -> " + " ou " - " selon le signe
 *   - coefficient 1 avec expo > 0 -> on n'ecrit pas le 1
 *     (ex : "X^3" au lieu de "1*X^3")
 *   - exposant 0                  -> seul le coefficient apparait
 *   - exposant 1                  -> "X" au lieu de "X^1"
 *
 * Le reste (lecture, construction de la liste) est inchange par
 * rapport a la Q2.
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
       Construit la liste chainee des monomes en respectant les signes.
       On accumule les maillons en queue pour preserver l'ordre de
       lecture (ce qui rend la verification visuelle plus facile). */
    private static Maillon polynome() throws IOException {
        int signe = 1;
        if (car == '-') {
            signe = -1;
            lire();
        }
        Maillon tete = monome();
        tete.coef *= signe;

        Maillon queue = tete;
        while (car == '+' || car == '-') {
            int s = (car == '-') ? -1 : 1;
            lire();
            Maillon m = monome();
            m.coef *= s;
            queue.suivant = m;
            queue = m;
        }
        return tete;
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

    public static void main(String[] args) throws IOException {
        lire();
        Maillon p = polynome();
        if (car != '\n' && car != -1)
            erreur("caractere inattendu apres le polynome");
        System.out.print("polynome reconnu : ");
        affiche(p);
        System.out.println();
    }
}
