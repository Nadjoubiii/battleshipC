#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define GRID_SIZE 10
// Define constants for ship types for clarity and use in the game logic
#define TYPE_CARRIER    0
#define TYPE_BATTLESHIP 1
#define TYPE_CRUISER    2
#define TYPE_SUBMARINE  3
#define TYPE_DESTROYER  4

typedef struct {
    char type;    // Type of ship (e.g., 'C' for Carrier)
    int size;     // Size of the ship
    int hits;     // Tracks hits for each ship
} Ship;

typedef struct {
    Ship carrier;
    Ship battleship;
    Ship cruiser;
    Ship submarine;
    Ship destroyer;
} Fleet;

void initGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '~';
        }
    }
}

void initializeFleet(Fleet *fleet) {
    fleet->carrier = (Ship){'C', 5, 0};
    fleet->battleship = (Ship){'B', 4, 0};
    fleet->cruiser = (Ship){'c', 3, 0};
    fleet->submarine = (Ship){'s', 3, 0};
    fleet->destroyer = (Ship){'d', 2, 0};
}

int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation) {
    for (int i = 0; i < ship->size; i++) {
        if ((orientation == 0 && (y + i >= GRID_SIZE || grid[x][y + i] != '~')) ||
            (orientation == 1 && (x + i >= GRID_SIZE || grid[x + i][y] != '~'))) {
            return 0;
        }
    }
    for (int i = 0; i < ship->size; i++) {
        if (orientation == 0) grid[x][y + i] = ship->type;
        if (orientation == 1) grid[x + i][y] = ship->type;
    }
    return 1;
}

void printGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    printf("   A B C D E F G H I J\n");
    for (int i = 0; i < GRID_SIZE; i++) {
        printf("%2d ", i + 1);
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

void initHeatmap(int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            heatmap[i][j] = 0;
        }
    }
    for (int index = 0; index < numShips; index++) {
        int shipSize = shipSizes[index];
        for (int row = 0; row < GRID_SIZE; row++) {
            for (int col = 0; col <= GRID_SIZE - shipSize; col++) {
                for (int pos = 0; pos < shipSize; pos++) {
                    heatmap[row][col + pos]++;
                }
            }
        }
        for (int col = 0; col < GRID_SIZE; col++) {
            for (int row = 0; row <= GRID_SIZE - shipSize; row++) {
                for (int pos = 0; pos < shipSize; pos++) {
                    heatmap[row + pos][col]++;
                }
            }
        }
    }
}

void updateHeatmapOnHit(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[], int numShips, int shipDestroyed, char opponentGrid[GRID_SIZE][GRID_SIZE]) {
    // Initially mark the hit cell to prevent re-targeting
    heatmap[x][y] = -999;

    if (shipDestroyed) {
        // Reset heatmap values around the destroyed ship
        for (int dx = -3; dx <= 3; dx++) {
            for (int dy = -3; dy <= 3; dy++) {
                int newX = x + dx;
                int newY = y + dy;
                if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) {
                    heatmap[newX][newY] = 0; // Completely reset the area around the destroyed ship
                }
            }
        }
    } else {
        // Check if there's a vertical or horizontal line of hits
        int vertical = 1, horizontal = 1;
        for (int i = 1; i < 3; i++) { // Check up to two cells away to determine orientation
            if (y - i >= 0 && opponentGrid[x][y - i] != opponentGrid[x][y]) vertical = 0;
            if (y + i < GRID_SIZE && opponentGrid[x][y + i] != opponentGrid[x][y]) vertical = 0;
            if (x - i >= 0 && opponentGrid[x - i][y] != opponentGrid[x][y]) horizontal = 0;
            if (x + i < GRID_SIZE && opponentGrid[x + i][y] != opponentGrid[x][y]) horizontal = 0;
        }

        // Increase probability in the confirmed orientation
        int directions[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}}; // Right, Left, Down, Up
        for (int i = 0; i < 4; i++) {
            int newX = x + directions[i][0];
            int newY = y + directions[i][1];
            if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE && heatmap[newX][newY] >= 0) {
                if ((i < 2 && horizontal) || (i >= 2 && vertical)) { // Only increase along the determined orientation
                    heatmap[newX][newY] += 3;
                }
            }
        }
    }
}

void updateHeatmapOnMiss(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[], int numShips) {
    heatmap[x][y] = 0;
    int maxShipSize = 0;
    for (int i = 0; i < numShips; i++) {
        if (shipSizes[i] > maxShipSize) {
            maxShipSize = shipSizes[i];
        }
    }
    for (int i = 1; i < maxShipSize; i++) {
        int directions[4][2] = {{0, i}, {0, -i}, {i, 0}, {-i, 0}};
        for (int j = 0; j < 4; j++) {
            int newX = x + directions[j][0];
            int newY = y + directions[j][1];
            if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE && heatmap[newX][newY] > 0) {
                heatmap[newX][newY] = max(0, heatmap[newX][newY] - 1);
            }
        }
    }
}

void printHeatmap(int heatmap[GRID_SIZE][GRID_SIZE]) {
    printf("Heatmap:\n");
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%2d ", heatmap[i][j]);
        }
        printf("\n");
    }
}

int main() {
    char opponentGrid[GRID_SIZE][GRID_SIZE];
    int heatmap[GRID_SIZE][GRID_SIZE];
    Fleet opponentFleet;
    int hits[5] = {0}; // Array to track hits on each ship
    int destroyed[5] = {0}; // Array to track if ships are destroyed

    initGrid(opponentGrid);
    initializeFleet(&opponentFleet);

    // Simulated "random" ship placements
    placeShip(opponentGrid, &opponentFleet.carrier, 1, 2, 0);
    placeShip(opponentGrid, &opponentFleet.battleship, 3, 5, 1);
    placeShip(opponentGrid, &opponentFleet.cruiser, 6, 1, 0);
    placeShip(opponentGrid, &opponentFleet.submarine, 5, 8, 1);
    placeShip(opponentGrid, &opponentFleet.destroyer, 0, 7, 0);

    int shipSizes[] = {opponentFleet.carrier.size, opponentFleet.battleship.size, opponentFleet.cruiser.size,
                       opponentFleet.submarine.size, opponentFleet.destroyer.size};
    initHeatmap(heatmap, shipSizes, 5);

    printf("Opponent's Ship Placement (For Testing):\n");
    printGrid(opponentGrid);
    printf("\nInitial Heatmap Based on Possible Ship Placements:\n");
    printHeatmap(heatmap);

    int guessX, guessY;
    char input[10];
    int maxVal = 0;

    while (1) {
        maxVal = 0;
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                if (heatmap[i][j] > maxVal) {
                    maxVal = heatmap[i][j];
                    guessX = i;
                    guessY = j;
                }
            }
        }
        printf("Heatmap suggests targeting: Row %d, Column %d\n", guessX + 1, guessY + 1);
        printf("Enter result ('hit' or 'miss' or 'quit'): ");
        scanf("%s", input);

        if (strcmp(input, "quit") == 0) {
            break;
        } else if (strcmp(input, "hit") == 0) {
            int shipType = opponentGrid[guessX][guessY] - 'A';  // Simple mapping assuming 'A', 'B', 'C', etc.
            hits[shipType]++;
            int shipDestroyed = (hits[shipType] == shipSizes[shipType]);
            if (shipDestroyed) {
                destroyed[shipType] = 1;  // Mark the ship as destroyed
            }
            updateHeatmapOnHit(heatmap, guessX, guessY, shipSizes, 5, shipDestroyed, opponentGrid);
        } else if (strcmp(input, "miss") == 0) {
            updateHeatmapOnMiss(heatmap, guessX, guessY, shipSizes, 5);
        }

        printHeatmap(heatmap);
    }

    return 0;
}