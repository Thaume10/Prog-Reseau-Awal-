#include "jeu.h"
#include <stdio.h>

void startGame(Partie *partie, int joueur, int sens)
{

    for (int i = 0; i < TAILLE_PLATEAU; i++)
    {
        partie->plateau[i] = 4;
    }
    partie->sens = sens;
    partie->joueurActuel = joueur + 1;
    if (partie->sens == 2)
    {
        partie->sens = -1;
    }
}

#include <stdio.h>
#include <stdlib.h>

char *afficher(const Partie *partie)
{
    // Déclarer une chaîne de caractères pour stocker la sortie
    char *output = (char *)malloc(1024 * sizeof(char)); // Allouer de l'espace pour la chaîne (ajuster la taille au besoin)

    if (output == NULL)
    {
        // Gestion de l'erreur d'allocation de mémoire
        return NULL;
    }

    // Commencer la construction de la chaîne
    sprintf(output, "Plateau de jeu:\n|");

    for (int i = 0; i < TAILLE_PLATEAU / 2; i++)
    {
        // Concaténer les valeurs au tableau
        sprintf(output, "%s%d|", output, partie->plateau[i]);
    }

    // Ajouter le séparateur
    sprintf(output, "%s\n|", output);

    for (int i = TAILLE_PLATEAU - 1; i > TAILLE_PLATEAU / 2 - 1; i--)
    {
        // Concaténer les valeurs au tableau
        sprintf(output, "%s%d|", output, partie->plateau[i]);
    }

    // Ajouter la ligne finale
    sprintf(output, "%s\n", output);

    // Retourner le tableau de caractères
    return output;
}

void jouerCoup(Partie *partie, int pos)
{
    int nbBille = 0;
    if (partie->joueurActuel == 2)
    {
        pos = 13 - pos;
    }
    nbBille = partie->plateau[pos - 1];
    partie->plateau[pos - 1] = 0;
    int i;
    if (partie->sens == 1)
    {
        i = 1;
    }
    else
    {
        i = -1;
    }
    while (nbBille > 0)
    {
        if (pos - 1 + i < 0)
        {
            i = 11 - pos + 1;
        }
        if ((pos - 1 + i) % 12 != pos - 1)
        {
            partie->plateau[(pos - 1 + i) % 12] += 1;
            nbBille--;
        }
        i += partie->sens;
    }
    recupPoints(partie, (pos - 1 + i - partie->sens) % 12);
}

void recupPoints(Partie *partie, int pos)
{
    int ok = peutCapturerSansAffamer(partie, pos);
    while ((partie->plateau[pos] == 2 || partie->plateau[pos] == 3) && ((partie->joueurActuel == 1 && pos >= 6 && pos < 12) || (partie->joueurActuel == 2 && pos < 6 && pos >= 0)) && !ok)
    {
        partie->points[partie->joueurActuel - 1] += partie->plateau[pos];
        partie->plateau[pos] = 0;
        pos = pos - partie->sens;
    }
}

int peutCapturerSansAffamer(const Partie *partie, int pos)
{
    int res = 0;
    while ((partie->plateau[pos] == 2 || partie->plateau[pos] == 3) && ((partie->joueurActuel == 1 && pos >= 6 && pos < 12) || (partie->joueurActuel == 2 && pos < 6 && pos >= 0)))
    {
        res += 1;
        pos = pos - partie->sens;
    }
    return res == 6;
}

int checkNourrir(const Partie *partie)
{
    int ok = -1; // Tout va bien
    int totalGrainesJoueur1 = 0;
    int totalGrainesJoueur2 = 0;
    for (int i = 0; i < TAILLE_PLATEAU; i++)
    {
        if (i >= 0 && i < 6)
        {
            totalGrainesJoueur1 += partie->plateau[i];
        }
        else
        {
            totalGrainesJoueur2 += partie->plateau[i];
        }
    }
    if (partie->joueurActuel == 2 && totalGrainesJoueur1 == 0)
    {
        ok = -2; // Coup impossible
        for (int i = 6; i < 12; i++)
        {
            if ((partie->plateau[i] * partie->sens) + i < 0)
            {
                i = 11 - (partie->plateau[i] * partie->sens);
            }

            if (((partie->plateau[i] * partie->sens) + i) % 12 >= 0 && ((partie->plateau[i] * partie->sens) + i) % 12 < 6)
            {
                ok = 13 - i;
            }
        }
    }
    else if (partie->joueurActuel == 1 && totalGrainesJoueur2 == 0)
    {
        ok = -2;
        for (int i = 0; i < 6; i++)
        {
            if ((partie->plateau[i] * partie->sens) + i < 0)
            {
                i = 11 - (partie->plateau[i] * partie->sens);
            }
            if (((partie->plateau[i] * partie->sens) + i) % 12 > 5 && ((partie->plateau[i] * partie->sens) + i) % 12 < 12)
            {
                ok = i;
            }
        }
    }
    return ok;
}

int capturePossible(const Partie *partie)
{
    int ok = 0;
    if (partie->joueurActuel == 2)
    {
        for (int i = 6; i < 12; i++)
        {
            int position = (partie->plateau[i] * partie->sens) + i;
            if (position < 0)
            {
                position = 11 - position; // Vous devriez inverser le signe ici
            }
            if (position >= 0 && position <= 11)
            { // Vérification des limites du tableau
                int target = partie->plateau[position];
                if (target != 0)
                {
                    ok = 1;
                }
            }
        }
    }
    else if (partie->joueurActuel == 1)
    {
        for (int i = 0; i < 6; i++)
        {
            int position = (partie->plateau[i] * partie->sens) + i;
            if (position < 0)
            {
                position = 11 - position; // Vous devriez inverser le signe ici
            }
            if (position >= 0 && position <= 11)
            { // Vérification des limites du tableau
                int target = partie->plateau[position];
                if (target != 0)
                {
                    ok = 1;
                }
            }
        }
    }
    return ok;
}

int finJeu(Partie *partie)
{
    int totalGrainesJoueur1 = 0;
    int totalGrainesJoueur2 = 0;
    int i;

    // Calcul du total de graines pour chaque joueur
    for (i = 0; i < TAILLE_PLATEAU; i++)
    {
        if (i >= 0 && i < 6)
        {
            totalGrainesJoueur1 += partie->plateau[i];
        }
        else
        {
            totalGrainesJoueur2 += partie->plateau[i];
        }
    }

    // Règle 1 : Un joueur a capturé au moins 25 graines
    if (partie->points[0] > 25)
    {
        return 1; // Joueur 1 a gagné
    }
    else if (partie->points[1] > 25)
    {
        return 2; // Joueur 2 a gagné
    }

    int nourrir = checkNourrir(partie);
    // Règle 3 : Fin par famine
    if (totalGrainesJoueur1 == 0 && nourrir == -2)
    {
        partie->points[1] += totalGrainesJoueur2;
    }
    if (totalGrainesJoueur2 == 0 && nourrir == -2)
    {
        partie->points[0] += totalGrainesJoueur1;
    }

    if (nourrir == -2)
    {
        if (partie->points[0] > partie->points[1])
        {
            return 1;
        }
        else if (partie->points[0] < partie->points[1])
        {
            return 2;
        }
        else
        {
            return -1;
        }
    }
    // Si aucune des conditions précédentes n'est remplie, le jeu continue
    return 0;
}
