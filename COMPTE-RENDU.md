# NMV: Gestion de la mémoire virtuelle

## Étudiant 
Guillaume Vacherias : 21204391

## Exercice 1

### Question 1 
*Dans le cadre de ce projet on supposera toujours qu’un niveau intermédiaire de la table des pages est mappée par une identitée (il est accessible par une adresse virtuelle égale à son adresse physique). Pourquoi cette supposition est-elle importante ?*

Il est important de connaître l'adresse physique de l'entrée des pages des tables afin de pouvoir accéder à ses tables. Cette adresse est stocké dans le registre CR3 du processeur. Cette adresse sera utilisé par la MMU pour un accés de la table. En effet, pour écrire directement à une adresse précise, il est nécessaire de connaître l'entrée physique de la table des pages. Il est nécéssaire d'avoir une fonction d'identité qui s'applique au niveau de table de page qui stocke les autres entrée (adresses) des tables de pages.

$addr_{phys} = cr3 + addr_{virt} \times 0x8$

Chaque niveau intermédiaire de la table des pages stockent une adresse physique d'un niveau supérieur de la table.

### Question 2
*Combien y a-t-il d’entrée par niveau de table des pages pour une architecture x86 64 bits ? Comment la MMU détermine-t-elle si une entrée d’un niveau de table des pages est valide ? Si elle est terminale ?*

* Il existe une entrée par niveau de table des pages. On peut remarquer qu'avec la commande *info resgisters* qu'on a des registres *CR0*, *CR1*, *CR2*, *CR3*. 
* On peut déterminer si une entrée d'un niveau de table des pages est valide grace au dernier bit de l'adresse nommé **P**. Si **P** est à 1 alors l'entrée est valide si le **P** est à 0 alors elle n'est pas présent dans la mémoire.
* On sait que la page est terminale si c'est une huge page donc avec le bit **PS**, on peut connaître la nature de la page.

## Exercice 2

### Question 1
*Etant donné une adresse virtuelle vaddr à mapper et la hauteur courante dans la table des pages lvl (avec lvl = 4 pour le niveau indiqué dans le CR3), donnez la formule qui indique l’index de l’entrée correspondante dans le niveau courant*

Les adresse sont sur 64 bits dont 48 décrivant l'adresses dans la page table. Il faut donc masquer et faire un décalage pour récupérer l'adresse.
```clike=
if(ctx->pgt == 4)
	idx = ((vaddr & 0xFF8000000000) >> 39);
if(ctx->pgt == 3)
    idx = ((vaddr & 0x7FC0000000) >> 30);
if(ctx->pgt == 2)
    idx = ((vaddr & 0x3FE00000) >> 21);
if(ctx->pgt == 1)
    idx = ((vaddr & 0x1FF000) >> 12);
```

## Exercice 3
Un processus = 1 table de pages
### Question 1
La page n'est pas encore alloué, ce qui provoque une page fault.

### Question 2
*La premiere étape de la création d’une nouvelle tâche en mémoire est de dériver la table des pages courante en une nouvelle table des pages. Expliquez quelles plages d’adresses de la table courante doivent être conservées dans la nouvelle table et pourquoi.*

On doit conserver la plage d'adresses du noyaux 0x0 à 0x40000000. La table PML3 conserve les adresses d'entrée du noyau. Elle comporte une taille de 1Giga. On veut que les processus ait accés l'espace du noyau donc garder la table des pages stockant le noyau. Il faut alors donner un verrou PML2 pour garantir que 2 processus n'exécutent pas en même temps une partie du code noyau.

### Question 3
*Donnez les adresses virtuelles de début et de fin du payload et du bss d’une tâche, calcul ́ees en fonction du modèle memoire et des champs d’une tâche ctx*
* *bss* : initialise à 0 les variables globales non initialisées on doit donc blanchir la page alloué pour le bss

| Address                     | Memory model |
| --------------------------- | ------------ |
| payload_size ~ bss_end      | Bss          |
| 0x2000000000 ~ payload_size | Payload      |
| 0x00 ~ 0x2000000000         | Noyau        |

Se servir des champs task pour déterminer les tailles : 
* addr_payload = 0x2000000000
* addr_payload_end = load_end_paddr - load_paddr
* addr_bss = ctx->load_vaddr + ctx->load_end_paddr - ctx->load_paddr
* addr_bss_end 

Stock l'ancien CR3 du PLM3 pour l'entrée du noyau

### Question 5
Changer le CR3 à la nouvelle page pour commuter de process

## Exercice 4 

### Question 2
*A cette étape du projet, l’exéecution de Rackdoll doit afficher sur le moniteur qu’une faute de page se produit à l’adresse virtuelle 0x1ffffffff8. Étant donnée le modèle mémoire, indiquez ce qui provoque la faute de page. D’après vous, cette faute est-elle causée par un accès mémoire légitime ?*

L'adresse virtuelle 0x1FFFFFFFF8 se trouve d'après le modéle mémoire entre 0x40000000 et 0x2000000000 ce qui représente la User stack. Cette stack n'est pas encore alloué à l'accés.

### Question 3
D’après le modèle memoire de Rackdoll, la pile d’une tâche utilisateur a une taille de 127 GiB, c’est à dire bien plus que la m ́emoire physique disponible dans la machine virtuelle. La pile est donc allouée de manière paresseuse. Expliquez en quoi consiste l’allocation paresseuse.

L'allocation paresseuse est une allocation qui n'est qu'effectuer au moment où on en a besoin. En effet, l'allocation renvoi tout d'abord l'adresse utilisable et ne sera vraiment alloué quand on voit une opération utilisant cette cette dernière. L'allocation ne sera jamais exécuté si l'adresse renvoyé lors de l'appel au malloc n'est jamais utilisée. 
