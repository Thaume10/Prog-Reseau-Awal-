#ifndef JEU_H
#define JEU_H

#define TAILLE_PLATEAU 12

typedef struct
{
    int plateau[TAILLE_PLATEAU];
    int sens;
    int points[2];
    int joueurActuel;

} Partie;

void startGame(Partie *partie, int joueur, int sens);
char *afficher(const Partie *partie);
void jouerCoup(Partie *partie, int pos);
void recupPoints(Partie *partie, int pos);
int peutCapturerSansAffamer(const Partie *partie, int pos);
int checkNourrir(const Partie *partie);
int capturePossible(const Partie *partie);
int finJeu(Partie *partie);

#endif /* JEU_H */
