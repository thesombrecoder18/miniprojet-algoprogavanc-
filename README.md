# Mini-Projet : Représentation des polynômes et leurs opérations
## Document de compréhension partagée — équipe

> **But de ce document** : que tous les membres du groupe aient **exactement la même compréhension** du projet avant de commencer à coder. Chaque section commence par une explication **simple** (image / analogie), puis une explication **technique** (vocabulaire et code).

> **Consigne du prof** : Implémentation en **C puis en Java**. Rendu et présentation dans **1 semaine**. Travail **authentique** exigé.

---

## Table des matières

1. [Vue d'ensemble du projet](#0)
2. [Q1 — Analyseur syntaxique](#q1)
3. [Q2 — Codage en mémoire](#q2)
4. [Q3 — Affichage](#q3)
5. [Q4 — Tri par degré décroissant](#q4)
6. [Q5 — Évaluation](#q5)
7. [Q6 — Opérations arithmétiques](#q6)
8. [Q7 — Garbage collector](#q7)
9. [Q8 — Versions récursives](#q8)
10. [Stratégie d'équipe & planning](#strat)
11. [Spécificités C vs Java](#cjava)

---

<a name="0"></a>
## 0. Vue d'ensemble du projet

### Version simple

Un **polynôme**, c'est une recette qui mélange une lettre mystère **X** avec des nombres.

Par exemple :
```
-4.5 * X^5 + 2 * X^4 + X^3 - X + 123.0
```

Chaque petit bout (comme `2 * X^4`) s'appelle un **monôme**. Un monôme contient **deux infos** :
- un **coefficient** (le nombre devant) → ici `2`
- un **exposant** (la puissance de X) → ici `4`

Le projet, c'est construire un programme qui sait **jouer avec les polynômes** comme une calculatrice joue avec les nombres : les lire, les afficher, les additionner, les multiplier, les diviser, calculer leur valeur quand X = 3 (par exemple).

Il y a **deux mondes** dans ce projet :
- le monde du **texte** (ce que l'utilisateur tape : `"3*X^2 + 5"`)
- le monde de la **mémoire** (la façon dont le programme range ça pour calculer)

Tout le projet, c'est passer de l'un à l'autre et faire des opérations.

### Version technique

Le projet est découpé en **8 questions cumulatives** qui aboutissent à un programme C (puis Java) capable de :
- Parser une chaîne représentant un polynôme à une variable selon une grammaire BNF donnée.
- Stocker chaque polynôme dans une liste chaînée triée par degré décroissant.
- Exécuter les opérations arithmétiques `+`, `−`, `×`, `÷` (avec reste).
- Évaluer le polynôme en un point.
- Gérer la mémoire dynamique via un GC mark-and-sweep.

Chaque question **étend** la précédente — on ne réécrit pas, on enrichit.

---

<a name="q1"></a>
## 1. Question 1 — Analyseur syntaxique

### Version simple

Un **analyseur syntaxique** = un correcteur d'orthographe pour polynômes.
Tu lui donnes un texte, il te dit :
- soit "C'est bien écrit"
- soit "Erreur ici, j'attendais autre chose"

Il **lit caractère par caractère**, comme un robot très patient qui a une liste de règles dans la tête.

#### Les règles (la grammaire)

Une **grammaire** = la liste des règles de bonne écriture. Comme la grammaire du français : "une phrase = sujet + verbe + complément". Sauf qu'ici c'est :

| Règle | Lecture |
|-------|---------|
| `polynôme → [-] monôme { (+\|-) monôme }` | Un polynôme = un signe `-` optionnel, puis au moins un monôme, puis autant de `+monôme` ou `-monôme` qu'on veut. |
| `monôme → nombre*xpuissance \| xpuissance \| nombre` | Un monôme c'est l'une de ces 3 formes. |
| `xpuissance → X \| X^naturel` | C'est `X` tout seul ou `X^entier`. |
| `naturel → chiffre {chiffre}` | Un entier positif (au moins un chiffre). |
| `nombre → naturel [.{chiffre}]` | Un entier ou un décimal. |
| `chiffre → 0\|1\|2\|...\|9` | Un chiffre. |

**Attention** : les méta-symboles `[ ]`, `{ }`, `|` ne sont **pas** dans le polynôme. Ils décrivent juste la règle.
- `[ ]` = facultatif
- `{ }` = répétable (0, 1, ou plusieurs fois)
- `|` = ou bien

### Version technique

#### Technique : descente récursive (recursive descent parser)

Une fonction par non-terminal de la grammaire :

```c
void polynome(void);
void monome(void);
void xpuissance(void);
void nombre(void);
void naturel(void);
```

Mécanique partagée :
- Un caractère courant `char car;` (variable globale).
- Une fonction `lire()` qui avance d'un caractère.
- Une fonction `erreur(const char *msg)` qui affiche le message et fait `exit(1)`.

Correspondance grammaire → code :
- `[ X ]` → `if (...)`
- `{ X }` → `while (...)`
- `A | B` → `if/else` selon le caractère lu

#### Ce que la Q1 produit
- Programme qui lit l'entrée et **valide** la syntaxe.
- Aucune structure de données pour l'instant — pas de stockage du polynôme.

---

<a name="q2"></a>
## 2. Question 2 — Codage en mémoire

### Version simple

À la Q1, le programme **lit** mais **oublie tout**. Maintenant on veut qu'il **se souvienne** du polynôme pour pouvoir s'en servir.

On le range comme un **train** : chaque wagon (= un **maillon**) contient un monôme, et un crochet vers le wagon suivant. Le dernier wagon a un crochet vers "rien".

Schéma pour `3*X^2 + 5*X + 7` :
```
tête → [coef=3, expo=2] → [coef=5, expo=1] → [coef=7, expo=0] → NULL
```

Le polynôme nul (= zéro) → liste vide → `NULL`.

### Version technique

#### Structure de données

```c
typedef struct maillon {
    double         coef;
    int            expo;
    struct maillon *suivant;
} MAILLON;

typedef MAILLON *POINTEUR;
```

#### Modification de l'analyseur

Les fonctions de la Q1 changent de signature :
```c
POINTEUR polynome(void);
POINTEUR monome(void);
// ...
```

Chaque fonction **alloue** des maillons (`malloc`) et **renvoie** l'adresse du début du bout reconnu.

**Règle stricte du sujet** : *"rien de ce qui constituait l'analyseur ne doit disparaître"*. On **étend** le code de la Q1, on ne le réécrit pas.

#### Équivalent Java

```java
class Maillon {
    double coef;
    int    expo;
    Maillon suivant;
}
```
En Java : pas de `malloc`, on fait `new Maillon()`. La liste vide est représentée par `null`.

---

<a name="q3"></a>
## 3. Question 3 — Affichage

### Version simple

Une fonction qui prend une liste chaînée et l'**affiche** lisiblement.

Exemple : la liste `[(-4.5, 5), (2, 4), (1, 3), (-1, 1), (123, 0)]` devient :
```
-4.5*X^5 + 2*X^4 + X^3 - X + 123
```

C'est la **Q1 à l'envers** : Q1 transforme du texte en mémoire ; Q3 transforme la mémoire en texte.

### Version technique

#### Cas particuliers à gérer pour un affichage propre

| Cas | Affichage |
|-----|-----------|
| Liste vide | `0` |
| `coef == 1`, `expo > 0` | `X^expo` (pas `1*X^expo`) |
| `coef == -1`, `expo > 0` | `-X^expo` |
| `expo == 0` | juste le nombre |
| `expo == 1` | `X` (pas `X^1`) |
| Coef négatif après le 1er monôme | ` - ` au lieu de ` + -` |

Signature C :
```c
void affiche(POINTEUR p);
```

---

<a name="q4"></a>
## 4. Question 4 — Tri par degré décroissant

### Version simple

Jusqu'ici, les monômes pouvaient être dans n'importe quel ordre. Maintenant on impose : **plus grand exposant en tête**, on descend ensuite.

Pourquoi ? Parce que :
1. C'est la **convention math** standard.
2. **Surtout** : ça facilitera énormément les opérations de la Q6 (parcours en parallèle de deux listes triées = "fermeture éclair").

### Version technique

On modifie l'**insertion** d'un nouveau maillon : insertion triée classique.

```c
POINTEUR insererTrié(POINTEUR liste, double coef, int expo);
```

#### Bonus important : fusion des doublons

Si l'utilisateur tape `3*X^2 + 5*X^2`, on fusionne en `8*X^2` directement à l'insertion. Si après fusion `coef == 0`, on retire le maillon (un terme nul est inutile).

---

<a name="q5"></a>
## 5. Question 5 — Évaluation

### Version simple

"Pour ce polynôme P et cette valeur x, combien vaut P(x) ?"

Exemple : `P = 2X² + 3X + 5` et `x = 4` → P(4) = 2×16 + 3×4 + 5 = **49**.

### Version technique

```c
double eval(POINTEUR p, double x);
```

Méthode directe :
```
total ← 0
pour chaque maillon m de p :
    total ← total + m.coef × pow(x, m.expo)
renvoyer total
```

(Optionnel — schéma de **Horner** si on veut être élégant, mais non demandé.)

---

<a name="q6"></a>
## 6. Question 6 — Opérations arithmétiques

### Version simple

On veut que le programme sache faire `P + Q`, `P - Q`, `P × Q` et `P ÷ Q` (avec reste).

**Deux règles d'or** imposées par le sujet :

**Règle 1** : On **ne touche JAMAIS** aux polynômes d'entrée. Le résultat est un polynôme **tout neuf** fait avec des maillons fraîchement alloués.

**Règle 2** : On **ne libère RIEN**. Pas de `free` dans ces fonctions. C'est volontaire — on règlera la mémoire à la Q7.

Conséquence : le code de la Q6 sera **simple, court, fiable**, mais **gourmand** en mémoire. C'est exprès.

### Version technique

```c
POINTEUR plus    (POINTEUR a, POINTEUR b);
POINTEUR moins   (POINTEUR a, POINTEUR b);
POINTEUR fois    (POINTEUR a, POINTEUR b);
POINTEUR quotient(POINTEUR a, POINTEUR b, POINTEUR *reste);
```

#### Addition (`plus`) — algorithme "fusion / merge"

Listes triées par degré décroissant → parcours parallèle :

| Cas | Action |
|-----|--------|
| `a.expo > b.expo` | copier le maillon de `a`, avancer dans `a` |
| `a.expo < b.expo` | copier le maillon de `b`, avancer dans `b` |
| `a.expo == b.expo` | additionner les coefs ; si somme ≠ 0, créer un maillon ; avancer des deux côtés |
| une liste finie | recopier ce qui reste de l'autre |

#### Soustraction (`moins`)

Soit la même technique avec `coef_a - coef_b`, soit `plus(a, neg(b))`.

#### Multiplication (`fois`) — formule récursive donnée par le sujet

> P × Q = (aₙXⁿ + P') × (bₘXᵐ + Q') = aₙbₘX^(n+m) + aₙXⁿ × Q' + P' × Q

Cas de base : si `a` ou `b` est `NULL` → renvoyer `NULL`.
Cas général : prendre les deux têtes, calculer trois morceaux, **les additionner avec `plus`**.

#### Division (`quotient`) — division euclidienne posée

Algo classique :
```
q ← polynôme nul
r ← copie de a
tant que degré(r) >= degré(b) :
    t ← monôme tel que t × tête(b) = tête(r)
    q ← plus(q, t)
    r ← moins(r, fois(t, b))
renvoyer q ; *reste = r
```

**Attention** : vérifier `b ≠ NULL` (sinon division par zéro).

**Note** : le `*reste` (pointeur sur pointeur) est nécessaire car C ne peut renvoyer qu'**une** valeur. En **Java**, on s'en sortira différemment — voir [section C vs Java](#cjava).

---

<a name="q7"></a>
## 7. Question 7 — Garbage collector

### Version simple

Le problème : à la Q6 on alloue tout le temps sans jamais libérer → la mémoire se remplit de **maillons fantômes** que plus personne n'utilise. À long terme, le programme grossit jusqu'à planter.

La solution : un **éboueur automatique** qui de temps en temps :
1. Regarde quels maillons sont **encore utiles** (encore atteignables depuis tes variables).
2. **Libère** tous les autres.

C'est l'algo classique **Mark & Sweep** :
- **Mark** (marquer) : on pose un drapeau sur chaque maillon utile.
- **Sweep** (balayer) : on parcourt **tous** les maillons existants. Ceux marqués → on garde (et on enlève le drapeau). Ceux non marqués → poubelle.

**Règle d'or de la Q7** : ce mécanisme travaille **"en sous-sol"** — on ne touche **PAS** aux fonctions de la Q6. Le GC est un module ajouté par-dessus, invisible.

### Version technique

#### Ajouts à la structure

```c
typedef struct maillon {
    double         coef;
    int            expo;
    struct maillon *suivant;
    struct maillon *general;   // ← NOUVEAU : chaînage de tous les maillons existants
    int            utile;      // ← NOUVEAU : drapeau de marquage
} MAILLON;
```

Chaque maillon a maintenant **deux chaînages indépendants** :
- `suivant` → maillon suivant **dans son polynôme** (utilisé partout depuis la Q2).
- `general` → maillon suivant **dans l'inventaire global** (utilisé uniquement par le GC).

#### Variables globales pour le GC

```c
POINTEUR tousLesMaillons = NULL;   // tête de la grande liste de tous les maillons
POINTEUR polyUtile[100];           // racines : polynômes considérés utiles
int      nbPolyUtile = 0;
```

#### Truc de design pour ne PAS modifier la Q6

On centralise toutes les allocations dans **une seule fonction** :

```c
POINTEUR nouveauMaillon(double coef, int expo, POINTEUR suivant) {
    POINTEUR m = malloc(sizeof(MAILLON));
    m->coef = coef;
    m->expo = expo;
    m->suivant = suivant;
    m->utile = 0;
    m->general = tousLesMaillons;   // ← le seul ajout pour la Q7
    tousLesMaillons = m;
    return m;
}
```

Tout le code de Q2/Q4/Q6 utilise déjà cette fonction → on **ne modifie qu'elle** pour la Q7. C'est l'idée "en sous-sol".

#### Le GC

```c
void recycler(void) {
    // Phase 1 : marquer
    for (int i = 0; i < nbPolyUtile; i++)
        for (POINTEUR m = polyUtile[i]; m != NULL; m = m->suivant)
            m->utile = 1;

    // Phase 2 : balayer
    POINTEUR nouvelleTete = NULL;
    POINTEUR m = tousLesMaillons;
    while (m != NULL) {
        POINTEUR suiv = m->general;   // sauvegarder AVANT free
        if (m->utile) {
            m->utile = 0;
            m->general = nouvelleTete;
            nouvelleTete = m;
        } else {
            free(m);
        }
        m = suiv;
    }
    tousLesMaillons = nouvelleTete;
}
```

#### Cas Java
En Java, le GC est **automatique** (intégré au langage). Mais le sujet demande explicitement de **mettre en place** ce mécanisme. À discuter avec le prof : on peut soit l'implémenter à la main (en utilisant `WeakReference` ou en simulant), soit expliquer pourquoi il est inutile en Java. Voir [section C vs Java](#cjava).

---

<a name="q8"></a>
## 8. Question 8 — Versions récursives

### Version simple

Une fonction **récursive** = une fonction qui **s'appelle elle-même** sur un problème plus petit, jusqu'à arriver à un cas trivial qu'elle résout directement.

Pour `plus`, l'idée :
> "Je regarde juste les deux **têtes** de a et b. Je décide de la tête du résultat, et je rappelle `plus` sur **ce qui reste**."

C'est la **même logique** que la version itérative (boucle), mais le code **ressemble à la définition mathématique**.

### Version technique

```c
POINTEUR plus(POINTEUR a, POINTEUR b) {
    if (a == NULL) return copie(b);
    if (b == NULL) return copie(a);

    if (a->expo > b->expo)
        return nouveauMaillon(a->coef, a->expo, plus(a->suivant, b));
    if (a->expo < b->expo)
        return nouveauMaillon(b->coef, b->expo, plus(a, b->suivant));

    // exposants égaux
    double s = a->coef + b->coef;
    POINTEUR queue = plus(a->suivant, b->suivant);
    if (s == 0) return queue;
    return nouveauMaillon(s, a->expo, queue);
}
```

`moins` : même structure, mais on nie les coefs venant de `b` et on calcule `a.coef - b.coef` quand les exposants coïncident.

**Limitation théorique** : un polynôme avec un million de monômes → débordement de pile. Pas un souci pour ce projet.

---

<a name="strat"></a>
## 9. Stratégie d'équipe & planning (1 semaine)

### Ordre conseillé

1. **Q1** d'abord, complète. Tester avec **plein** d'entrées valides ET invalides.
2. **Q3** **avant** de finir la Q2 — sinon on programme à l'aveugle, sans pouvoir vérifier la liste.
3. Q2 → Q4 → Q5 (assez rapides une fois Q1+Q3 OK).
4. **Q6** : `plus` d'abord (le plus simple, et tout le reste le réutilise), tester ; puis `moins`, `fois`, `quotient`.
5. **Q7** uniquement quand Q6 marche bien. Sinon on débogue deux choses à la fois.
6. **Q8** en dernier (réécriture courte si la Q6 est solide).

### Découpage possible entre membres

| Membre | C | Java |
|--------|---|------|
| A | Q1 + Q2 + Q3 | Q1 + Q2 + Q3 |
| B | Q4 + Q5 + Q8 | Q4 + Q5 + Q8 |
| C | Q6 (plus, moins, fois, quotient) | Q6 |
| D | Q7 (GC) + tests d'intégration | Q7 (ou explication) + tests |

**Important** : tout le monde doit comprendre **tout** le code (présentation orale exigée).

### Fichier de tests partagé

Maintenir un fichier `tests.txt` avec des lignes du type :
```
Entrée: 3*X^2 + 5*X + 7
Évaluation en X=2: 29
P+P: 6*X^2 + 10*X + 14
P*P: 9*X^4 + 30*X^3 + 67*X^2 + 70*X + 49
```
Tout le monde l'utilise pour valider sa partie.

### Authenticité du travail

Le prof a précisé **"travail authentique"**. Concrètement :
- **Pas de copier-coller** d'un projet trouvé en ligne.
- Maîtriser **chaque ligne** du code rendu (présentation orale).
- Préparer pour la soutenance : être capable d'expliquer **n'importe quelle partie** du code à n'importe quel membre.

---

<a name="cjava"></a>
## 10. Spécificités C vs Java

### 10.0 Différences générales (valables partout)

| Aspect | C | Java |
|--------|---|------|
| Structure du maillon | `struct` + `typedef` | `static class Maillon` (nichée) |
| Allocation | `malloc(sizeof(...))` | `new Maillon(...)` |
| Liste vide / pas de pointeur | `NULL` | `null` |
| Test de chiffre | `isdigit(c)` (depuis `<ctype.h>`) | `Character.isDigit(c)` |
| Lecture d'un caractère | `getchar()` (renvoie `int`, `EOF` à la fin) | `System.in.read()` (renvoie `int`, `-1` à la fin) |
| Erreur fatale | `fprintf(stderr,...) ; exit(1)` | `System.err.println(...) ; System.exit(1)` |
| Affichage | `printf` | `System.out.print` (+ `StringBuilder` pour les longues constructions) |
| Format `%g` (sans `.0`) | natif via `printf("%g", d)` | il faut écrire un mini utilitaire `fmt(double)` |
| `pow(x, n)` | `<math.h>` | `Math.pow(x, n)` |
| Exceptions I/O | implicites | doivent être déclarées : `throws IOException` |
| Nom de fichier | libre | doit correspondre à la classe publique (`Analyseur.java`) |
| Arguments du `main` | `int argc, char **argv` ; les vrais args commencent à `argv[1]` | `String[] args` ; les vrais args commencent à `args[0]` |
| Conversion en double | `atof(argv[1])` | `Double.parseDouble(args[0])` |

---

### 10.1 Q1 — Analyseur syntaxique

| | C | Java |
|---|---|---|
| Lecture | `getchar()` | `System.in.read()` |
| EOF | `EOF` | `-1` |
| Test chiffre | `isdigit(car)` | `Character.isDigit(car)` |
| Propagation des erreurs I/O | implicite | `throws IOException` sur **toutes** les méthodes qui lisent |

Aucune autre différence structurelle : la descente récursive est identique dans les deux langages.

---

### 10.2 Q2 — Codage en mémoire

| | C | Java |
|---|---|---|
| Type maillon | `typedef struct {...} MAILLON; typedef MAILLON *POINTEUR;` | `static class Maillon { ... }` |
| Allocation | `malloc(sizeof(MAILLON))` | `new Maillon(coef, expo, suiv)` |
| Constructeur | inexistant en C, on remplit les champs après | méthode dédiée `Maillon(...)` |
| Polynôme nul | `NULL` | `null` |
| `return` après `erreur()` | facultatif (compilateur muet) | **obligatoire** même si "inatteignable", sinon erreur de compilation |

Java exige toujours un `return` même si la méthode appelle juste avant `erreur()` qui termine. C'est une particularité du compilateur Java qui ne sait pas que `System.exit(1)` ne revient jamais.

---

### 10.3 Q3 — Affichage

| | C | Java |
|---|---|---|
| Construction de la chaîne | `printf` successifs directement | `StringBuilder` puis `print` final |
| Format des doubles | `%g` (sans `.0` automatique) | `fmt()` utilitaire maison |

En C, on imprime au fil de l'eau. En Java, comme les `String` sont **immutables**, on accumule dans un `StringBuilder` pour éviter d'allouer une nouvelle `String` à chaque concaténation. Le résultat visuel est identique.

---

### 10.4 Q4 — Tri par degré décroissant

**Aucune différence d'algorithme.** L'`insererMaillon` récursive est portée à l'identique : `liste->expo` devient `liste.expo`, `m->coef` devient `m.coef`, c'est tout.

Petite différence philosophique : en C, un maillon "absorbé" par fusion devient un fantôme jusqu'à la Q7. **En Java, le GC intégré le récupère automatiquement** dès qu'aucune référence ne le pointe plus.

---

### 10.5 Q5 — Évaluation

| | C | Java |
|---|---|---|
| Lecture de l'argument | `argv[1]` | `args[0]` |
| Vérif de présence | `if (argc >= 2)` | `if (args.length >= 1)` |
| Conversion en double | `atof(argv[1])` | `Double.parseDouble(args[0])` |
| `pow(x, n)` | boucle de multiplications (évite `<math.h>`) | identique, ou `Math.pow(x, n)` |

Le décalage d'index dans `args` est le piège classique : en C, `argv[0]` est le nom du programme ; en Java, `args[0]` est déjà le **premier argument utilisateur**.

---

### 10.6 Q6 — Opérations arithmétiques

| | C | Java |
|---|---|---|
| Signature de `quotient` | `quotient(a, b, POINTEUR *reste)` | `DivisionResultat quotient(a, b)` |
| Renvoi de plusieurs valeurs | pointeur sur pointeur | classe résultat |
| `ajouterEnQueue` | `void ajouterEnQueue(POINTEUR *tete, POINTEUR *queue, ...)` | classe interne `Constructeur` qui encapsule `tete` et `queue` |

**Le grand changement de la Q6 entre les deux langages.** En C, on utilise des pointeurs sur pointeurs (`POINTEUR *`) pour modifier des variables externes. En Java, on **encapsule** ces variables dans un petit objet :

```java
// En C : pointeur sur pointeur
void quotient(POINTEUR a, POINTEUR b, POINTEUR *reste);
// utilisation
POINTEUR r;
POINTEUR q = quotient(a, b, &r);

// En Java : classe résultat
static class DivisionResultat { Maillon quotient; Maillon reste; }
DivisionResultat quotient(Maillon a, Maillon b);
// utilisation
DivisionResultat dr = quotient(a, b);
Maillon q = dr.quotient;
Maillon r = dr.reste;
```

C'est plus lisible, plus typé, plus Java.

Pour `ajouterEnQueue`, on fait pareil : on crée une mini-classe `Constructeur` qui contient `tete` et `queue` comme champs internes, et expose une méthode `ajouter(coef, expo)`.

---

### 10.7 Q7 — Garbage collector

| | C | Java |
|---|---|---|
| Centralisation des allocations | fonction `nouveauMaillon` | **constructeur** `Maillon(...)` |
| Effet "ajout à `tousLesMaillons`" | dans `nouveauMaillon` | dans le constructeur (effet de bord) |
| Action sur les maillons non marqués | `free(m)` | `m.general = null` (détacher) |
| Récupération réelle | immédiate via `free()` | différée par le GC intégré de la JVM |
| Utilité pratique | indispensable | **purement pédagogique** |

**La différence la plus importante de tout le projet.**

En C, le GC est nécessaire : sans lui, les fuites mémoire sont réelles. En Java, le GC du langage fait déjà le travail. On implémente quand même le mécanisme parce que :
1. Le sujet l'exige.
2. C'est l'occasion de **démontrer qu'on comprend** l'algorithme mark-and-sweep, qui est précisément celui de la JVM.

**Astuce Java** : on profite du fait que le constructeur de `Maillon` est appelé à **chaque** `new Maillon(...)`. On y ajoute l'effet de bord "rejoindre `tousLesMaillons`", et **toute la Q6 fonctionne sans modification**. C'est l'équivalent exact du C où on modifiait uniquement `nouveauMaillon`.

```java
Maillon(double coef, int expo, Maillon suivant) {
    this.coef = coef;
    this.expo = expo;
    this.suivant = suivant;
    this.utile = false;
    // L'unique modification pour la Q7 :
    this.general = tousLesMaillons;
    tousLesMaillons = this;
}
```

**Réponse à donner au prof** s'il demande pourquoi on simule un GC en Java :
> *"Pour respecter le sujet et démontrer qu'on comprend l'algorithme mark-and-sweep, qui est exactement celui utilisé par certaines JVMs. En pratique dans un vrai projet Java, on laisserait le GC intégré faire son travail."*

---

### 10.8 Q8 — Versions récursives

**Aucune différence structurelle.** Les fonctions `plusRec` et `moinsRec` se traduisent ligne à ligne entre C et Java. Les deux versions souffrent de la même **limitation théorique** : un polynôme à un million de monômes provoquerait un débordement de pile.

---

### Synthèse : où sont les vraies différences

| Question | Différence | Importance |
|----------|------------|-----------|
| Q1 | I/O et exceptions | mineure |
| Q2 | struct vs class | esthétique |
| Q3 | `StringBuilder` au lieu de `printf` | mineure |
| Q4 | aucune | — |
| Q5 | index `args[0]` au lieu de `argv[1]` | mineure |
| Q6 | classe `DivisionResultat` au lieu de `POINTEUR *reste` | **importante** |
| Q7 | constructeur Java fait le boulot de `nouveauMaillon` ; pas de `free()` | **importante** |
| Q8 | aucune | — |

Les **vraies** différences se concentrent sur **Q6 et Q7**. Pour les autres questions, la traduction est essentiellement syntaxique.

---

## Checklist finale avant rendu

- [ ] Q1–Q8 implémentées **en C**
- [ ] Q1–Q8 implémentées **en Java**
- [ ] Tests partagés validés sur les deux versions
- [ ] Code commenté **à minima** (juste le pourquoi non évident)
- [ ] README ou rapport bref expliquant la structure des fichiers
- [ ] Tous les membres savent expliquer **toutes** les parties (soutenance)
- [ ] Pas de code copié — chaque ligne maîtrisée
