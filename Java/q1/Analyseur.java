/*
 * Question 1 : Analyseur syntaxique pour les polynomes (version Java).
 *
 * Grammaire reconnue (BNF) :
 *   polynome   -> [-] monome { (+|-) monome }
 *   monome     -> nombre * xpuissance | xpuissance | nombre
 *   xpuissance -> X | X^naturel
 *   naturel    -> chiffre {chiffre}
 *   nombre     -> naturel [.{chiffre}]
 *   chiffre    -> 0|1|2|3|4|5|6|7|8|9
 *
 * Le programme lit l'entree standard caractere par caractere et
 * verifie qu'elle respecte la grammaire ci-dessus. En cas d'erreur,
 * il affiche un message sur stderr et arrete le programme avec
 * System.exit(1). Sinon, il affiche "polynome reconnu".
 *
 * Cette version Java reproduit fidelement l'analyseur C : meme
 * algorithme (descente recursive), memes regles sur les espaces
 * (autorises entre tokens, interdits dans un nombre).
 *
 * Compilation : javac Analyseur.java
 * Execution   : echo "3*X^2 + 5" | java Analyseur
 */

import java.io.IOException;

public class Analyseur {

    /* Caractere courant : celui qu'on est en train d'examiner.
       On stocke un int (et non un char) pour pouvoir comparer a -1
       (la valeur retournee par System.in.read() en fin de fichier). */
    private static int car;

    /* Avance d'un caractere dans 'car', en sautant les espaces et
       tabulations. Apres l'appel, 'car' contient soit un caractere
       significatif, soit '\n', soit -1 (EOF). */
    private static void lire() throws IOException {
        do {
            car = System.in.read();
        } while (car == ' ' || car == '\t');
    }

    /* Affiche un message d'erreur sur stderr et arrete le programme. */
    private static void erreur(String msg) {
        System.err.println("Erreur de syntaxe : " + msg);
        System.exit(1);
    }

    /* naturel -> chiffre {chiffre}
       Lit un entier non signe : au moins un chiffre, suivi de zero
       ou plusieurs autres chiffres. On utilise System.in.read() en
       direct (et non lire()) pour interdire les espaces a l'interieur
       d'un nombre : "3 5" ne doit pas etre accepte comme "35". */
    private static void naturel() throws IOException {
        if (!Character.isDigit(car))
            erreur("chiffre attendu");
        do {
            car = System.in.read();
        } while (Character.isDigit(car));
        while (car == ' ' || car == '\t')
            car = System.in.read();
    }

    /* nombre -> naturel [.{chiffre}]
       Lit un nombre eventuellement decimal. Pas d'espace autorise
       autour du point ni entre les chiffres decimaux. */
    private static void nombre() throws IOException {
        naturel();
        if (car == '.') {
            car = System.in.read();
            while (Character.isDigit(car))
                car = System.in.read();
            while (car == ' ' || car == '\t')
                car = System.in.read();
        }
    }

    /* xpuissance -> X | X^naturel
       Lit la lettre X, eventuellement suivie de ^ et d'un naturel. */
    private static void xpuissance() throws IOException {
        if (car != 'X')
            erreur("'X' attendu");
        lire();
        if (car == '^') {
            lire();
            naturel();
        }
    }

    /* monome -> nombre * xpuissance | xpuissance | nombre
       On choisit la bonne regle selon le caractere courant. */
    private static void monome() throws IOException {
        if (car == 'X') {
            xpuissance();
        } else if (Character.isDigit(car)) {
            nombre();
            if (car == '*') {
                lire();
                xpuissance();
            }
            /* Sinon, monome reduit a un nombre : rien a faire. */
        } else {
            erreur("debut de monome attendu (X ou chiffre)");
        }
    }

    /* polynome -> [-] monome { (+|-) monome }
       Un polynome est une suite non vide de monomes separes par '+'
       ou '-', le tout pouvant etre precede d'un signe '-'. */
    private static void polynome() throws IOException {
        if (car == '-')
            lire();
        monome();
        while (car == '+' || car == '-') {
            lire();
            monome();
        }
    }

    public static void main(String[] args) throws IOException {
        /* Amorcage : on charge le premier caractere avant l'analyse. */
        lire();
        polynome();
        /* Apres le polynome, on doit etre en fin de ligne ou de
           fichier ; tout autre caractere est une erreur. */
        if (car != '\n' && car != -1)
            erreur("caractere inattendu apres le polynome");
        System.out.println("polynome reconnu");
    }
}
