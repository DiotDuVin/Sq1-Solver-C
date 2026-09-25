/*
- A-H pour les coins, 1-8 pour les arêtes
- '-' ou '/'
- Position résolue : A1B2C3D45E6F7G8H-
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int  uint;
typedef unsigned char uchar;

uint turbostock=0;

// Demi-couches (HalfLayer)

typedef struct {
    char Name[7];
    char Turns[6];
    int Corners, Edges, Pieces;
} HalfLayer;

// Couches complètes (Layer)

typedef struct {
    char Name[13];
    uchar Turn;
    int TurnParity;   // (en vrai c'est un bool)
    uchar NextLeft, NextRight;
} Layer;

// Tables globales

HalfLayer hl[13];
Layer layers[13][13];

/* Table de pruning : ShapeTable[t1][t2][b1][b2][pm]
   bit 6:0 = profondeur+1 (0=non initialisé)
   bit 7 = un twist change la parité*/
char ShapeTable[13][13][13][13][2];

// Tables de permutation (phase 2)
char PermTable[2][40320];
uint PermTwistTable[40320];
uint PermTopTable[40320];
uint PermBotTable[40320];

// Liste de mouvements

#define MAXMOVES 200
char moveList[MAXMOVES];
int moveLen = 0;
int moveNbTwist = 0;

void push_move(char m){
    moveList[moveLen++] = m;
    if (m == 0) moveNbTwist++;
}
void pull_move(void){
    if (moveLen > 0) {
        if (moveList[--moveLen] == 0) moveNbTwist--;
    }
}

// Helpers

uint umax(uint a, uint b) { return a > b ? a : b; }
uint umin(uint a, uint b) { return a < b ? a : b; }

// Permutations

void Num2Perm(char *perm, char chr, uint num, int n)
{
    char w[9];
    int a, b;
    uint c;
    for (a = 0; a < n; a++) w[a] = a + chr;
    for (a = 0; a < n; a++) {
        c = num / (n - a);
        b = (int)(num - c * (n - a));
        num = c;
        perm[a] = w[b];
        while (++b < n) w[b-1] = w[b];
    }
}

uint Perm2Num(const char *perm, int n)
{
    uint p = 0;
    int a, b, c;
    for (a = n-1; a >= 0; a--) {
        c = 0;
        for (b = a+1; b < n; b++)
            if (perm[b] < perm[a]) c++;
        p = p * (n - a) + c;
    }
    return p;
}

// HalfLayer

void hl_set(HalfLayer *h, const char *s1, const char *s2)
{
    int i = 0;
    strncpy(h->Name,  s1, 6); h->Name[6]  = 0;
    strncpy(h->Turns, s2, 5); h->Turns[5] = 0;
    h->Corners = h->Edges = 0;
    while (h->Name[i]) {
        if (h->Name[i] == 'E') h->Edges++;
        else                   h->Corners++;
        i++;
    }
    h->Corners >>= 1;
    h->Pieces = h->Edges + h->Corners;
}

// Layer 

void layer_set(Layer *lay, const HalfLayer *h1, const HalfLayer *h2)
{
    int i, j, t;
    strncpy(lay->Name, h1->Name, 6);
    strncpy(lay->Name+6, h2->Name, 6);
    lay->Name[12] = 0;

    t = (h1->Pieces + h2->Pieces) & 1;
    // trouver le plus petit tour commun
    i = j = 0; lay->Turn = 6;
    while (h1->Turns[i] != 0 && h2->Turns[j] != 0) {
        if      (h1->Turns[i] > h2->Turns[j]) j++;
        else if (h1->Turns[i] < h2->Turns[j]) i++;
        else { lay->Turn = (uchar)(h1->Turns[i] - '0'); break; }
    }

    // parité de la permutation
    int par = 0;
    if (t == 0) {
        for (t = 0; t < lay->Turn; t++) {
            if (lay->Name[11-t] != 'c') par = !par;
        }
    }
    lay->TurnParity = par;
}

// Initialisation des couches

void InitLayers(void)
{
    int a, b, c, d, t;
    char nm[13]; nm[12] = 0;

    hl_set(&hl[0],  "EEEEEE", "12345");
    hl_set(&hl[1],  "CcEEEE", "1234");
    hl_set(&hl[2],  "ECcEEE", "1235");
    hl_set(&hl[3],  "EECcEE", "1245");
    hl_set(&hl[4],  "EEECcE", "1345");
    hl_set(&hl[5],  "EEEECc", "2345");
    hl_set(&hl[6],  "CcCcEE", "124");
    hl_set(&hl[7],  "CcECcE", "134");
    hl_set(&hl[8],  "CcEECc", "234");
    hl_set(&hl[9],  "ECcCcE", "135");
    hl_set(&hl[10], "ECcECc", "235");
    hl_set(&hl[11], "EECcCc", "245");
    hl_set(&hl[12], "CcCcCc", "24");

    for (a = 0; a < 13; a++)
        for (b = 0; b < 13; b++)
            layer_set(&layers[a][b], &hl[a], &hl[b]);

    // tables de transition (NextLeft/NextRight)
    for (a = 0; a < 13; a++) {
        for (b = 0; b < 13; b++) {
            t = layers[a][b].Turn;
            for (c = 0; c < t;  c++) nm[c] = layers[a][b].Name[c+12-t];
            for (c = t; c < 12; c++) nm[c] = layers[a][b].Name[c-t];
            for (c = 0; c < 13; c++) {
                for (d = 0; d < 13; d++) {
                    if (strncmp(layers[c][d].Name, nm, 12) == 0) {
                        layers[a][b].NextLeft  = (uchar)c;
                        layers[a][b].NextRight = (uchar)d;
                        c = 13; break;
                    }
                }
            }
        }
    }

    /* parité des twists */
    for (a=0;a<13;a++) for(b=0;b<13;b++) for(c=0;c<13;c++) for(d=0;d<13;d++)
        ShapeTable[a][b][c][d][0] = ShapeTable[a][b][c][d][1] = 0;

    for (b = 0; b < 13; b++) {
        if (hl[b].Pieces & 1) {
            for (c = 0; c < 13; c++) {
                if (hl[c].Pieces & 1) {
                    for (a=0;a<13;a++) for(d=0;d<13;d++)
                        ShapeTable[a][b][c][d][0] =
                        ShapeTable[a][b][c][d][1] = (char)128;
                }
            }
        }
    }
}

/* Initialisation de la ShapeTable 
Mode twist : un "move" = twist + rotations libres des couches */

void InitShapeTable(void)
{
    int a,b,c,d,e, i, a2,b2,c2,d2,e2, t;
    char l;

    ShapeTable[7][7][10][10][0] |= 1;
    ShapeTable[10][10][7][7][0] |= 1;
    ShapeTable[7][7][7][7][1] |= 1;
    ShapeTable[10][10][10][10][1] |= 1;

    l = 0;
    do {
        l++; i = 0;
        for(a=0;a<13;a++)for(b=0;b<13;b++)for(c=0;c<13;c++)for(d=0;d<13;d++)for(e=0;e<2;e++){
            if ((ShapeTable[a][c][b][d][e] & 127) == l) {
                // essayer twist
                e2 = (ShapeTable[a][c][b][d][e] & 128) ? 1-e : e;
                if ((ShapeTable[a][b][c][d][e2] & 127) == 0) {
                    i++;
                    ShapeTable[a][b][c][d][e2] += l+1;

                    a2=a; b2=b; c2=c; d2=d;
                    // tourner couche haute
                    do {
                        e2 = (layers[a2][b2].TurnParity) ? 1-e2 : e2;
                        t=a2;
                        a2=layers[a2][b2].NextLeft;
                        b2=layers[t ][b2].NextRight;
                        if ((ShapeTable[a2][b2][c2][d2][e2] & 127) == 0) {
                            ShapeTable[a2][b2][c2][d2][e2] += l+1; i++;
                        }
                        // tourner couche basse
                        do {
                            e2 = (layers[c2][d2].TurnParity) ? 1-e2 : e2;
                            t=c2;
                            c2=layers[c2][d2].NextLeft;
                            d2=layers[t ][d2].NextRight;
                            if ((ShapeTable[a2][b2][c2][d2][e2] & 127) == 0) {
                                ShapeTable[a2][b2][c2][d2][e2] += l+1; i++;
                            }
                        } while (c2!=c || d2!=d);
                    } while (a2!=a || b2!=b);
                }
            }
        }
    } while (i);
}

// Initialisation de la PermTable

void InitPermTable(void)
{
    uint a, b, i;
    char t, c, d, l;
    char Pos[9];

    for (a = 0; a < 40320; a++) {
        PermTable[0][a] = PermTable[1][a] = 0;
        PermTwistTable[a] = PermBotTable[a] = PermTopTable[a] = 0;
    }

    for (a = 0; a < 40320; a++) {
        // twist
        Num2Perm(Pos, 'A', a, 8);
        t=Pos[2]; Pos[2]=Pos[4]; Pos[4]=t;
        t=Pos[3]; Pos[3]=Pos[5]; Pos[5]=t;
        PermTwistTable[a] = Perm2Num(Pos, 8);

        // rotation couche haute
        Num2Perm(Pos, 'A', a, 8);
        t=Pos[3]; Pos[3]=Pos[2]; Pos[2]=Pos[1]; Pos[1]=Pos[0]; Pos[0]=t;
        PermTopTable[a] = Perm2Num(Pos, 8);

        // rotation couche basse
        Num2Perm(Pos, 'A', a, 8);
        t=Pos[7]; Pos[7]=Pos[6]; Pos[6]=Pos[5]; Pos[5]=Pos[4]; Pos[4]=t;
        PermBotTable[a] = Perm2Num(Pos, 8);
    }

    // remplissage de PermTable depuis les positions résolues
    b = 0;
    for (c = 0; c < 4; c++) {
        for (d = 0; d < 4; d++) {
            PermTable[0][b] = 1;
            b = PermBotTable[b];
        }
        b = PermTopTable[b];
    }

    l = 0;
    do {
        l++; i = 0;
        for (a = 0; a < 40320; a++) for (t = 0; t < 2; t++) {
            if (PermTable[1-t][a] == l) {
                b = PermTwistTable[a];
                if (PermTable[t][b] == 0) {
                    for (c = 0; c < 4; c++) {
                        if (PermTable[t][b] == 0) { i++; PermTable[t][b] = l+1; }
                        for (d = 0; d < 4; d++) {
                            if (PermTable[t][b] == 0) { i++; PermTable[t][b] = l+1; }
                            b = PermBotTable[b];
                        }
                        b = PermTopTable[b];
                    }
                }
            }
        }
    } while (i);
}

// Position de la  Phase 1

typedef struct {
    char pos[25];   // pièces (chaîne)
    char t1,t2,b1,b2;
    char pm;        // parité de permutation (0/1)
    char ml;        // couche médiane : +1=carré, -1=kite, 0=ignoré
} Pos1;

int findhl(const char *shp)
{
    int a;
    for (a = 0; a < 13; a++)
        if (strncmp(hl[a].Name, shp, 6) == 0) return a;
    return -1;
}

void TurnLayer(char *ps, int d)
{
    char c;
    switch (d) {
    case 1:
        c=ps[11]; 
        for (int i=11; i>=1; i--){
            ps[i]=ps[i-1];
        }
        ps[0]=c;
        break;
    case 2:
        c=ps[11];
        for (int i=11; i>=3; i-=2){
            ps[i]=ps[i-2];
        }
        ps[1]=c;
        c=ps[10];
        for (int i=10; i>=2; i-=2){
            ps[i]=ps[i-2];
        }
        ps[0]=c;
        break;
    case 3:
        c=ps[11];
        for (int i=11; i>=5; i-=3){
            ps[i]=ps[i-3];
        }
        ps[2]=c;
        c=ps[10];
        for (int i=10; i>=4; i-=3){
            ps[i]=ps[i-3];
        }
        ps[1]=c;
        c=ps[9];
        for (int i=9; i>=3; i-=3){
            ps[i]=ps[i-3];
        }
        ps[0]=c;
        break;
    case 4:
        c=ps[11]; ps[11]=ps[7]; ps[7]=ps[3]; ps[3]=c;
        c=ps[10]; ps[10]=ps[6]; ps[6]=ps[2]; ps[2]=c;
        c=ps[9]; ps[9]=ps[5]; ps[5]=ps[1]; ps[1]=c;
        c=ps[8]; ps[8]=ps[4]; ps[4]=ps[0]; ps[0]=c;
        break;
    case 5:
        c=ps[11];
        for (int i=11; i!=4; i=(i+7) % 12){
            ps[i]=ps[(i+7) % 12];
        }
        ps[4]=c;
        break;
    case 6:
        for (int i=11; i>5; i--){
            c=ps[i];
            ps[i]=ps[i-6];
            ps[i-6]=c;
        }
        break;
    case 7:
        c=ps[11];
        for (int i=11; i!=6; i=(i+5) % 12){
            ps[i]=ps[(i+5) % 12];
        }
        ps[6]=c;
        break;
    case 8:
        c=ps[11]; ps[11]=ps[3]; ps[3]=ps[7]; ps[7]=c;
        c=ps[10]; ps[10]=ps[2]; ps[2]=ps[6]; ps[6]=c;
        c=ps[9]; ps[9]=ps[1]; ps[1]=ps[5]; ps[5]=c;
        c=ps[8]; ps[8]=ps[0]; ps[0]=ps[4]; ps[4]=c;
        break;
    case 9:
        c=ps[11]; ps[11]=ps[2]; ps[2]=ps[5]; ps[5]=ps[8]; ps[8]=c;
        c=ps[10]; ps[10]=ps[1]; ps[1]=ps[4]; ps[4]=ps[7]; ps[7]=c;
        c=ps[9]; ps[9]=ps[0]; ps[0]=ps[3]; ps[3]=ps[6]; ps[6]=c;
        break;
    case 10:
        c=ps[11]; ps[11]=ps[1]; ps[1]=ps[3]; ps[3]=ps[5]; ps[5]=ps[7]; ps[7]=ps[9]; ps[9]=c;
        c=ps[10]; ps[10]=ps[0]; ps[0]=ps[2]; ps[2]=ps[4]; ps[4]=ps[6]; ps[6]=ps[8]; ps[8]=c;
        break;
    case 11:
        c=ps[0]; 
        for (int i=0; i!=11; i++){
            ps[i]=ps[i+1];
        }     
        ps[11]=c;
        break;
    }
}

void TwistLayers(Pos1 *p)
{
    char c;
    c=p->pos[11];
    p->pos[11]=p->pos[17];
    p->pos[17]=c;
    c=p->pos[10];
    p->pos[10]=p->pos[16];
    p->pos[16]=c;
    c=p->pos[9];
    p->pos[9]=p->pos[15];
    p->pos[15]=c;
    c=p->pos[8];
    p->pos[8]=p->pos[14];
    p->pos[14]=c;
    c=p->pos[7];
    p->pos[7]=p->pos[13];
    p->pos[13]=c;
    c=p->pos[6];
    p->pos[6]=p->pos[12];
    p->pos[12]=c;
}

void Pos1_Initialise(Pos1 *p, char inipos[])
{
    char shape[25], prm[17];
    int a, b;
    p->pos[24] = 0;
    shape[24] = 0;
    prm[16] = 0;
    memcpy(p->pos, inipos, 24); p->pos[24] = 0;
    b = 0;
    /* 'a' is used as index in both pos and shape.
       For a corner pair (AA), a++ skips the 2nd char so shape[a]='C',shape[a+1]='c'
       and a ends up pointing to the 2nd char of the pair. */
    for (a = 0; a < 24; a++) {
        prm[b++] = p->pos[a];
        if (p->pos[a] >= '1' && p->pos[a] <= '8') {
            shape[a] = 'E';
        } else {
            shape[a] = 'C';
            a++;
            shape[a] = 'c';
        }
    }
    p->t1 = (char)findhl(shape);
    p->t2 = (char)findhl(shape+6);
    p->b1 = (char)findhl(shape+12);
    p->b2 = (char)findhl(shape+18);

    p->ml = (inipos[24]=='-') ? 1 : (inipos[24]=='/') ? -1 : 0;

    p->pm = 0;
    for (a = 0; a < 16; a++)
        for (b = a+1; b < 16; b++)
            if (prm[a] > prm[b]) p->pm ^= 1;
}

int Pos1_Top(Pos1 *p)
{
    int c, d;
    p->pm = layers[p->t1][p->t2].TurnParity ? 1-p->pm : p->pm;
    d = layers[p->t1][p->t2].Turn;
    c = p->t1;
    p->t1 = layers[p->t1][p->t2].NextLeft;
    p->t2 = layers[c][p->t2].NextRight;
    TurnLayer(p->pos, d);
    return d;
}

int Pos1_Bottom(Pos1 *p)
{
    int c, d;
    p->pm = layers[p->b1][p->b2].TurnParity ? 1-p->pm : p->pm;
    d = layers[p->b1][p->b2].Turn;
    c = p->b1;
    p->b1 = layers[p->b1][p->b2].NextLeft;
    p->b2 = layers[c][p->b2].NextRight;
    TurnLayer(p->pos+12, d);
    return d;
}

void Pos1_Twist(Pos1 *p)
{
    char b;
    p->pm = (ShapeTable[p->t1][p->t2][p->b1][p->b2][0] & 128) ? 1-p->pm : p->pm;
    b = p->t2; p->t2 = p->b1; p->b1 = b;
    p->ml = -p->ml;
    TwistLayers(p);
}

int Pos1_Depth(Pos1 *p)
{
    return (ShapeTable[p->t1][p->t2][p->b1][p->b2][p->pm] & 127) - 1;
}


//Position Phase 2


typedef struct {
    uint edgeperm, cornperm;
    int TopEdgeFirst; //bool
    int BotEdgeFirst; //bool
    char ml;
} Pos2;

void Pos2_Initialise(Pos2 *p2, Pos1 *p1)
{
    char prm[9];
    int a, b;
    char *inipos = p1->pos;
    prm[8] = 0;

    p2->ml = p1->ml;

    // coins
    for (a = 0; a < 8; a++) prm[a] = inipos[a*3+1];
    p2->cornperm = Perm2Num(prm, 8);

    // arêtes couche haute
    if (inipos[0] == inipos[1]) { a = 2; p2->TopEdgeFirst = 0; }
    else { a = 0; p2->TopEdgeFirst = 1; }
    for (b = 0; b < 4; a += 3, b++) prm[b] = inipos[a];

    // arêtes couche basse
    if (inipos[12] == inipos[13]) { a = 14; p2->BotEdgeFirst = 0; }
    else { a = 12; p2->BotEdgeFirst = 1; }
    for (; b < 8; a += 3, b++) prm[b] = inipos[a];

    p2->edgeperm = Perm2Num(prm, 8);
}

int Pos2_Depth(Pos2 *p2)
{
    uint l1, l2;
    switch (p2->ml) {
    case 1:
        l1 = (uint)PermTable[0][p2->edgeperm];
        l2 = (uint)PermTable[0][p2->cornperm];
        break;
    case 0:
        l1 = umin((uint)PermTable[0][p2->edgeperm],(uint)PermTable[1][p2->edgeperm]);
        l2 = umin((uint)PermTable[0][p2->cornperm], (uint)PermTable[1][p2->cornperm]);
        break;
    default:
        l1 = (uint)PermTable[1][p2->edgeperm];
        l2 = (uint)PermTable[1][p2->cornperm];
        break;
    }
    return (int)(umax(l1,l2)) - 1;
}

int Pos2_Top(Pos2 *p2)
{
    p2->TopEdgeFirst = !p2->TopEdgeFirst;
    if (p2->TopEdgeFirst) { p2->edgeperm = PermTopTable[p2->edgeperm]; return 1; }
    else { p2->cornperm = PermTopTable[p2->cornperm]; return 2; }
}
int Pos2_Bottom(Pos2 *p2)
{
    p2->BotEdgeFirst = !p2->BotEdgeFirst;
    if (p2->BotEdgeFirst) { p2->edgeperm = PermBotTable[p2->edgeperm]; return 1; }
    else { p2->cornperm = PermBotTable[p2->cornperm]; return 2; }
}
void Pos2_Twist(Pos2 *p2)
{
    p2->edgeperm = PermTwistTable[p2->edgeperm];
    p2->cornperm = PermTwistTable[p2->cornperm];
    p2->ml = -p2->ml;
}
int Pos2_CanTwist(Pos2 *p2)
{
    return p2->TopEdgeFirst == p2->BotEdgeFirst;
}
int Pos2_IsSolved(Pos2 *p2)
{
    return (p2->edgeperm == 0 && p2->cornperm == 0
            && p2->TopEdgeFirst == 0 && p2->BotEdgeFirst == 1
            && p2->ml >= 0);
}

// Moteur de recherche (Twist metric)

int MaxDepth;
int FoundSolution;
char Length1, Length2;
long Nodes1, Nodes2;

// Affichage d'un tour 
void print_turn(int a)
{
    if (a <= 6) printf(" %d", a);
    else printf("-%d", 12-a);
}

// Affichage de la solution 
void print_solution(void)
{
    int a, b, c;
    printf("Solution (%d twists) : ", moveNbTwist);
    turbostock=umax(turbostock,moveNbTwist);
    for (a = 0; a < moveLen; a++) {
        b = moveList[a];
        if (b == 0) {
            printf("/");
        } else if (b > 0) {
            if (a < moveLen-1 && (c = moveList[a+1]) < 0) {
                printf("(");
                print_turn(b);
                printf(",");
                print_turn(-c);
                printf(")");
                a++;
            } else {
                printf("(");
                print_turn(b);
                printf(",0)");
            }
        } else {
            printf("(0,");
            print_turn(-b);
            printf(")");
        }
    }
    printf("\n");
}

// Phase 2 

int Phase2(Pos2 ps2, char l2, char lm);

int StartPhase2(Pos1 *ps1, char lm)
{
    Pos2 p2;
    int r = 0;
    Pos2_Initialise(&p2, ps1);

    if (ps1->ml < 0) Length2 = 1;
    else Length2 = 0;

    while (Length2 <= MaxDepth - Length1 && r == 0) {
        r = Phase2(p2, Length2, lm);
        if (ps1->ml) Length2 += 2;
        else Length2++;
    }

    return r;
}

int Phase2(Pos2 ps2, char l2, char lm)
{
    int r = 0;
    char b_move, t_move;
    int b_acc, t_acc;

    int l = Pos2_Depth(&ps2);
    if (l > l2) return 0;

    Nodes2++;

    if (l2 == 0 && l == 0) {
        // essayer les rotations finales 
        Pos2 pt = ps2;
        t_acc = 0;
        do {
            if (t_acc) { push_move((char)t_acc); }
            Pos2 pb = pt;
            b_acc = 0;
            do {
                if (b_acc) { push_move((char)-b_acc); }
                if (Pos2_IsSolved(&pb)) {
                    printf("Solution (%d twists) :\n", moveNbTwist);
                    print_solution();
                    FoundSolution = 1;
                    MaxDepth = 0; // force l'arrêt de toute la recherche 
                    if (b_acc) pull_move();
                    if (t_acc) pull_move();
                    return 1;
                }
                if (b_acc) pull_move();
                b_move = (char)Pos2_Bottom(&pb);
                b_acc += b_move;
            } while (b_acc < 12);
            if (t_acc) pull_move();
            t_move = (char)Pos2_Top(&pt);
            t_acc += t_move;
        } while (t_acc < 12);
        return 0;
    } else if (lm > 0) {
        return 0;
    }

    t_acc = 0;
    {
        Pos2 pt = ps2;
        do {
            if (t_acc) push_move((char)t_acc);
            b_acc = 0;
            {
                Pos2 pb = pt;
                do {
                    if (t_acc || b_acc || lm) {
                        if (Pos2_CanTwist(&pb)) {
                            if (b_acc) push_move((char)-b_acc);
                            Pos2_Twist(&pb);
                            push_move(0);
                            r += Phase2(pb, l2-1, (b_acc>=6)?1:0);
                            pull_move();
                            Pos2_Twist(&pb);
                            if (b_acc) pull_move();
                            if (r) { if (t_acc) pull_move(); return r; }
                        }
                    }
                    b_move = (char)Pos2_Bottom(&pb);
                    b_acc += b_move;
                } while (b_acc < 12);
            }
            if (t_acc) pull_move();
            t_move = (char)Pos2_Top(&pt);
            t_acc += t_move;
        } while (t_acc < 12);
    }
    return r;
}

// Phase 1

int Phase1(Pos1 ps1, char l1, char lm);

int StartPhase1(Pos1 *initPos)
{
    Pos1 p1 = *initPos;
    Nodes1 = Nodes2 = 0;

    for (Length1 = 0; Length1 <= (char)MaxDepth; Length1++) {
        // Vérifier cohérence couche médiane et parité
        if (Length1 == (char)MaxDepth) {
            if ((p1.ml == -1 && (Length1 & 1) == 0)) break;
            if ((p1.ml ==  1 && (Length1 & 1) != 0)) break;
        }
        Phase1(p1, Length1, -1);
        if (FoundSolution) return 1;
    }
    return 0;
}

int Phase1(Pos1 ps1, char l1, char lm)
{
    int r = 0;
    char b_move, t_move;
    int b_acc, t_acc;
    Pos1 pt, pb;

    int l = Pos1_Depth(&ps1);
    if (l > l1) return 0;

    Nodes1++;
    if ((Nodes1 & 0xFFFFF) == 0)
        printf("Phase1 prof=%d nodes1=%ld  Phase2 nodes=%ld\r",
               (int)Length1, Nodes1, Nodes2);

    if (l == 0) {
        if (l1 == 0) {
            return StartPhase2(&ps1, lm);
        } else if (l1 < 2) return 0;
    }
    if (lm > 0) return 0;

    // Tourner couche haute
    t_acc = 0;
    pt = ps1;
    do {
        if (t_acc) push_move((char)t_acc);

        // Tourner couche basse
        b_acc = 0;
        pb = pt;
        do {
            if (t_acc || b_acc || lm < 0) {
                if (b_acc) push_move((char)-b_acc);
                // Twist
                Pos1_Twist(&pb);
                push_move(0);
                r += Phase1(pb, l1-1, (b_acc>=6)?1:0);
                pull_move();
                Pos1_Twist(&pb);
                if (b_acc) pull_move();
                if (r) { if (t_acc) pull_move(); return r; }
            }
            b_move = (char)Pos1_Bottom(&pb);
            b_acc += b_move;
        } while (b_acc < 12);

        if (t_acc) pull_move();
        t_move = (char)Pos1_Top(&pt);
        t_acc += t_move;
    } while (t_acc < 12);

    return r;
}

//Lecture de la position depuis la ligne de commande

int ReadPosition(const char *Input, Pos1 *p1)
{
    char Output[26];
    unsigned f = 0, g;
    int a, b;
    char c;
    int len = (int)strlen(Input);

    Output[24] = ' '; Output[25] = 0;

    if (len != 16 && len != 17) {
        fprintf(stderr, "Erreur : il faut une chaîne de 16 ou 17 caractères.\n");
        return 1;
    }

    b = 0;
    for (a = 0; a < 16 && b < 24; a++) {
        c = Input[a];
        if (c >= '1' && c <= '8') {
            Output[b++] = c;
            g = 1u << (c - '1');
        } else if ((c >= 'a' && c <= 'h') || (c >= 'A' && c <= 'H')) {
            if (b == 11) {
                fprintf(stderr, "Erreur : coin à cheval.\n"); return 1;
            }
            if (b == 5 || b == 17) {
                fprintf(stderr, "Erreur : coin bloque.\n"); return 1;
            }
            c |= 32;  /* minuscule */
            Output[b++] = (char)(c - 'a' + 'A');
            Output[b++] = (char)(c - 'a' + 'A');
            g = 1u << (c - 'a' + 8);
        } else {
            fprintf(stderr, "Erreur : caractère invalide '%c'.\n", c); return 1;
        }
        if (f & g) {
            fprintf(stderr, "Erreur : pièce en double.\n"); return 1;
        }
        f |= g;
    }

    if (Input[16]) {
        if (Input[16] == '/' || Input[16] == '-') Output[24] = Input[16];
        else { fprintf(stderr, "Erreur : 17e caractère doit être '/' ou '-'.\n"); return 1; }
    }

    Pos1_Initialise(p1, Output);
    return 0;
}

// main

/*
Générateur de positions aléatoires valides :
appliquer N mouvements aléatoires (rotations haut/bas + twists).
*/

// Melange aleatoire
void RandomPosition(Pos1 *p, unsigned int seed)
{
    // Position résolue en interne
    char solved[] = "AA1BB2CC3DD45EE6FF7GG8HH-";
    Pos1_Initialise(p, solved);

    unsigned int rng = seed;
#define RNG_NEXT() (rng = rng * 1664525u + 1013904223u)

    // 20-40 mouvements aleatoires
    RNG_NEXT();
    int nb_moves = 20 + (int)(rng % 21);
    int i;
    for (i = 0; i < nb_moves; i++) {
        RNG_NEXT();
        int action = rng % 3;   // 0=twist, 1=top, 2=bottom

        if (action == 0) {
            // Twist : seulement si la jonction est libre
            // On tente, sinon on tourne un peu d'abord
            int tries = 0;
            while (tries < 6) {
                // Vérifier si la jonction est libre
                Pos1 tmp = *p;
                Pos1_Twist(&tmp);
                if (tmp.t1 >= 0 && tmp.t2 >= 0 && tmp.b1 >= 0 && tmp.b2 >= 0) {
                    *p = tmp;
                    break;
                }
                // Jonction bloquée : tourner la couche haute d'un cran
                Pos1_Top(p);
                tries++;
            }
        } else if (action == 1) {
            // Rotation couche haute : 1 à 5 crans 
            RNG_NEXT();
            int steps = 1 + (int)(rng % 5);
            int acc = 0;
            int k;
            for (k = 0; k < steps && acc < 12; k++)
                acc += Pos1_Top(p);
        } else {
            // Rotation couche basse
            RNG_NEXT();
            int steps = 1 + (int)(rng % 5);
            int acc = 0;
            int k;
            for (k = 0; k < steps && acc < 12; k++)
                acc += Pos1_Bottom(p);
        }
    }
#undef RNG_NEXT
}


// Fonction de résolution d'une position (réinitialise l'état)

int SolveOne(Pos1 *p, int verbose)
{
    MaxDepth = 99;
    FoundSolution = 0;
    moveLen = 0;
    moveNbTwist = 0;

    if (verbose) {
        // Reconstruire la notation lisible depuis pos[] interne
        printf("Position interne : %.24s\n", p->pos);
    }

    StartPhase1(p);

    if (!FoundSolution && verbose)
        printf("Aucune solution trouvée.\n");

    return FoundSolution ? moveNbTwist : -1;
}


// main


void print_usage(const char *prog)
{
    printf("Usage:\n");
    printf("  %s <position>          résoudre une position\n", prog);
    printf("  %s -r [N] [seed]       tester N mélanges aléatoires (défaut: 100)\n\n", prog);
    printf("Position : 16 ou 17 caractères (A-H coins, 1-8 arêtes)\n");
    printf("Exemple résolu : A1B2C3D45E6F7G8H-\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) { print_usage(argv[0]); return 0; }

    printf("Initialisation des tables...\n");
    InitLayers();
    InitShapeTable();
    InitPermTable();
    printf("Tables prêtes.\n\n");

    // Mode aléatoire : -r [N] [seed]
    if (argv[1][0] == '-' && argv[1][1] == 'r') {
        int N    = (argc >= 3) ? atoi(argv[2]) : 100;
        unsigned int seed = (argc >= 4) ? (unsigned int)atoi(argv[3]) : 42u;
        if (N <= 0) N = 100;

        printf("%d mélanges aléatoires (seed=%u) \n\n", N, seed);
        
        int i;

        for (i = 0; i < N; i++) {
            Pos1 p;
            // Seed différente pour chaque mélange
            RandomPosition(&p, seed + (unsigned int)i * 2654435761u);

            printf("[%3d/%d] ", i+1, N);
            int t = SolveOne(&p, 0);
            //printf("%d",t);
            if (t < 0) {
                printf("ECHEC\n");
            }
        }
        printf("\nNombre de Dieu sur un échantillonnage de %d positions (seed = %d) : %d\n",N,seed,turbostock);
        return 0;
    }

    // Mode position unique 
    Pos1 p1;
    if (ReadPosition(argv[1], &p1)) return 1;
    printf("Recherche...\n");
    SolveOne(&p1, 1);
    if (!FoundSolution)
        printf("Aucune solution trouvée (vérifiez la position).\n");
    return 0;
}