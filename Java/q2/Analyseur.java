/*
 * Question 2 (Java) : Codage des polynomes en memoire.
 *
 * Reprend l'analyseur de la Q1 et y AJOUTE la construction d'une
 * liste chainee de maillons (coef, expo) representant le polynome
 * reconnu.
 *
 *   polynome   -> [-] monome { (+|-) monome }
 *   monome     -> nombre * xpuissance | xpuissance | nombre
 *   xpuissance -> X | X^naturel
 *   naturel    -> chiffre {chiffre}
 *   nombre     -> naturel [.{chiffre}]
 *
 * Le polynome nul est represente par 'null' (equivalent du NULL en
 * C). A la Q2, l'ordre des maillons reflete l'ordre de lecture ;
 * le tri par degre decroissant viendra a la Q4.
 *
 * Une fonction afficheDebug imprime la liste sous forme brute
 * "(coef, expo) -> ..." pour que l'on puisse verifier visuellement
 * que la construction est correcte. La fonction d'affichage propre
 * arrivera a la Q3.
 *
 * Note Java : la classe Maillon est definie comme une classe statique
 * nichee dans Analyseur. C'est l'equivalent de la struct C.
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
       Fonction de debug : affiche la liste brute pour verification
       visuelle. Sera remplacee par un affichage propre a la Q3.
       ------------------------------------------------------------------ */

    /* Petit utilitaire : formate un double comme C avec %g, c'est-a-dire
       sans le ".0" superflu pour les valeurs entieres (5.0 -> "5"). */
    private static String fmt(double d) {
        if (d == (long) d)
            return String.valueOf((long) d);
        return String.valueOf(d);
    }

    private static void afficheDebug(Maillon p) {
        if (p == null) {
            System.out.println("(liste vide)");
            return;
        }
        StringBuilder sb = new StringBuilder();
        while (p != null) {
            sb.append("(").append(fmt(p.coef)).append(", ")
              .append(p.expo).append(")");
            if (p.suivant != null)
                sb.append(" -> ");
            p = p.suivant;
        }
        sb.append(" -> NULL");
        System.out.println(sb.toString());
    }

    public static void main(String[] args) throws IOException {
        lire();
        Maillon p = polynome();
        if (car != '\n' && car != -1)
            erreur("caractere inattendu apres le polynome");
        System.out.println("polynome reconnu");
        System.out.print("liste construite : ");
        afficheDebug(p);
    }
}
