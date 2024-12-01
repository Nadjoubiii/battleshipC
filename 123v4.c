#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define GRID_SIZE 10
#define MAX_RADAR_USES 3
#define MAX_SMOKE_SCREENS 2

// Ship structure to define each ship type
typedef struct {
    char type;    // Type of ship (e.g., 'C' for Carrier)
    int size;     // Size of the ship
    int hits;     // Tracks hits for each ship
} Ship;

// Fleet structure to define a collection of ships for each player
typedef struct {
    Ship carrier;
    Ship battleship;
    Ship cruiser;
    Ship submarine;
    Ship destroyer;
    int shipsDestroyed;
    int artilleryUnlocked;
    int torpedoUnlocked;
    int artilleryUsed;
} Fleet;

// Smoke screen structure
typedef struct {
    int x, y;             // Coordinates of the smoke screen's top-left corner
    int turnsRemaining;   // Turns remaining before the smoke expires
} SmokeScreen;

// AI Targeting structure for hard difficulty
typedef struct {
    int x, y;             // Coordinates of the initial hit
    int isHit;            // Indicates if the AI has a confirmed hit
    int direction;        // Direction for targeting: 0-North, 1-South, 2-East, 3-West, -1 if undefined
    int radarUsed;        // Indicates if Radar Sweep has been used for this hit
    int referenceX;       // Current reference X coordinate for following the ship
    int referenceY;       // Current reference Y coordinate for following the ship
    int caseNum;          // Current case number (1, 2, or 3)
    int directionIndex;   // For case 3, index into the direction sequence
} aiTargetHard;

// Function prototypes
void initGrid(char grid[GRID_SIZE][GRID_SIZE]);
void printGrid(char grid[GRID_SIZE][GRID_SIZE]);
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE],
                  char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]);
void initializeFleet(Fleet *fleet);
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation);
void Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y, int *shipDestroyed);
void Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y);
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int index, char axis);
void RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses,
                SmokeScreen smokes[], int activeSmokeCount, int *enemyFound);
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y);
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount);
void initaiTargetHard(aiTargetHard *aiTargetHard);
void aiTurn(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, aiTargetHard *aiTargetHard,
            int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips, int *radarUses);

// Heatmap functions
void initHeatmap(int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips);
void updateHeatmapOnHit(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                        int numShips, int shipDestroyed, char opponentGrid[GRID_SIZE][GRID_SIZE]);
void updateHeatmapOnMiss(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                         int numShips);
void printHeatmap(int heatmap[GRID_SIZE][GRID_SIZE]);

// Helper function to get maximum of two integers
int max(int a, int b) {
    return (a > b) ? a : b;
}

// Initializes the grid with water '~'
void initGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '~';
        }
    }
}

// Prints the grid with column and row headers
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

// Displays grids for the current player and their view of the opponent's grid
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE],
                  char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]) {
    if (turn == 1) {
        printf("Your Grid (Player 1):\n");
        printGrid(grid1);
        printf("Opponent's Visible Grid:\n");
        printGrid(grid2for1);
    } else {
        printf("Your Grid (Player 2):\n");
        printGrid(grid2);
        printf("Opponent's Visible Grid:\n");
        printGrid(grid1for2);
    }
}

// Initializes the fleet with default ships
void initializeFleet(Fleet *fleet) {
    fleet->carrier = (Ship){'C', 5, 0};
    fleet->battleship = (Ship){'B', 4, 0};
    fleet->cruiser = (Ship){'c', 3, 0};
    fleet->submarine = (Ship){'s', 3, 0};
    fleet->destroyer = (Ship){'d', 2, 0};
    fleet->shipsDestroyed = 0;
    fleet->artilleryUnlocked = 0;
    fleet->torpedoUnlocked = 0;
    fleet->artilleryUsed = 0;
}

// Places a ship on the grid
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation) {
    for (int i = 0; i < ship->size; i++) {
        if (orientation == 0 && (y + i >= GRID_SIZE || grid[x][y + i] != '~')) return -1;
        if (orientation == 1 && (x + i >= GRID_SIZE || grid[x + i][y] != '~')) return -1;
    }
    for (int i = 0; i < ship->size; i++) {
        if (orientation == 0) grid[x][y + i] = ship->type;
        if (orientation == 1) grid[x + i][y] = ship->type;
    }
    return 0;
}

// Fires at a specific location
void Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y, int *shipDestroyed) {
    *shipDestroyed = 0; // Initialize to no ship destroyed

    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        printf("Invalid coordinates!\n");
        return;
    }
    if (grid[x][y] == '~') {
        grid[x][y] = 'm';  // Miss
        printf("Missed!\n");
    } else if (grid[x][y] != 'm' && grid[x][y] != 'h') {
        char shipType = grid[x][y];
        grid[x][y] = 'h';  // Hit
        printf("Hit!\n");

        int shipDestroyedLocal = 0;
        switch (shipType) {
            case 'C':
                fleet->carrier.hits++;
                if (fleet->carrier.hits == fleet->carrier.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 'B':
                fleet->battleship.hits++;
                if (fleet->battleship.hits == fleet->battleship.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 'c':
                fleet->cruiser.hits++;
                if (fleet->cruiser.hits == fleet->cruiser.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 's':
                fleet->submarine.hits++;
                if (fleet->submarine.hits == fleet->submarine.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 'd':
                fleet->destroyer.hits++;
                if (fleet->destroyer.hits == fleet->destroyer.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
        }

        if (shipDestroyedLocal) {
            printf("A ship has been destroyed!\n");
            *shipDestroyed = 1; // Notify that a ship has been destroyed

            if (!fleet->artilleryUnlocked && fleet->shipsDestroyed >= 1) {
                if (fleet->artilleryUsed == 0) {
                    fleet->artilleryUnlocked = 1;
                    printf("Artillery has been unlocked!\n");
                    fleet->artilleryUsed = 1;
                }
            }

            if (!fleet->torpedoUnlocked && fleet->shipsDestroyed >= 3) {
                fleet->torpedoUnlocked = 1;
                printf("Torpedo has been unlocked!\n");
            }
        }
    } else {
        printf("Already fired here!\n");
    }
}

// Executes an artillery strike
void Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y) {
    if (fleet->artilleryUnlocked) {
        printf("Artillery attack at %c%d!\n", 'A' + y, x + 1);
        int shipDestroyed;
        for (int i = x; i < x + 2 && i < GRID_SIZE; i++) {
            for (int j = y; j < y + 2 && j < GRID_SIZE; j++) {
                Fire(grid, fleet, i, j, &shipDestroyed);
            }
        }
        fleet->artilleryUnlocked = 0; // Reset artillery availability after use
    } else {
        printf("Artillery is not unlocked yet!\n");
    }
}

// Executes a torpedo strike on a column or row
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int index, char axis) {
    if (fleet->torpedoUnlocked) {
        int shipDestroyed;
        if (axis == 'C') {
            printf("Torpedo attack on column %c!\n", index + 'A');
            for (int i = 0; i < GRID_SIZE; i++) {
                Fire(grid, fleet, i, index, &shipDestroyed);
            }
        } else if (axis == 'R') {
            printf("Torpedo attack on row %d!\n", index + 1);
            for (int j = 0; j < GRID_SIZE; j++) {
                Fire(grid, fleet, index, j, &shipDestroyed);
            }
        }
        fleet->torpedoUnlocked = 0; // Reset torpedo availability after use
    } else {
        printf("Torpedo is not unlocked yet!\n");
    }
}

// Radar sweep implementation
void RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses,
                SmokeScreen smokes[], int activeSmokeCount, int *enemyFound) {
    for (int i = 0; i < activeSmokeCount; i++) {
        if (x >= smokes[i].x && x < smokes[i].x + 2 && y >= smokes[i].y && y < smokes[i].y + 2) {
            printf("Radar blocked by smoke.\n");
            *enemyFound = 0;
            return;
        }
    }

    if (*radarUses >= MAX_RADAR_USES) {
        printf("No more radar sweeps available.\n");
        *enemyFound = 0;
        return;
    }

    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        printf("Invalid coordinates for radar.\n");
        *enemyFound = 0;
        return;
    }

    *enemyFound = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (grid[x + i][y + j] != '~' && grid[x + i][y + j] != 'h' && grid[x + i][y + j] != 'm') {
                *enemyFound = 1;
            }
        }
    }

    (*radarUses)++;
    if (*enemyFound) {
        printf("Enemy ships found.\n");
    } else {
        printf("No enemy ships found.\n");
    }
}

// Smoke screen deployment
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y) {
    if (*activeSmokeCount >= MAX_SMOKE_SCREENS) {
        printf("Maximum smoke screens deployed.\n");
        return;
    }

    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        printf("Invalid coordinates for smoke.\n");
        return;
    }

    smokes[*activeSmokeCount].x = x;
    smokes[*activeSmokeCount].y = y;
    smokes[*activeSmokeCount].turnsRemaining = 3;
    (*activeSmokeCount)++;

    printf("Smoke screen deployed at %c%d.\n", 'A' + y, x + 1);
}

// Update smoke screens
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount) {
    for (int i = 0; i < *activeSmokeCount; i++) {
        smokes[i].turnsRemaining--;
        if (smokes[i].turnsRemaining <= 0) {
            printf("Smoke screen at %c%d expired.\n", 'A' + smokes[i].y, smokes[i].x + 1);
            for (int j = i; j < *activeSmokeCount - 1; j++) {
                smokes[j] = smokes[j + 1];
            }
            (*activeSmokeCount)--;
            i--;
        }
    }
}

// Initializes AI target
void initaiTargetHard(aiTargetHard *aiTargetHard) {
    aiTargetHard->isHit = 0;
    aiTargetHard->direction = -1;
    aiTargetHard->radarUsed = 0;
    aiTargetHard->referenceX = -1;
    aiTargetHard->referenceY = -1;
    aiTargetHard->caseNum = 0;
    aiTargetHard->x = -1;
    aiTargetHard->y = -1;
    aiTargetHard->directionIndex = 0;
}

// Heatmap initialization
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

// Heatmap update on hit
void updateHeatmapOnHit(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                        int numShips, int shipDestroyed, char opponentGrid[GRID_SIZE][GRID_SIZE]) {
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
        // Increase probability around the hit
        int directions[4][2] = {{0,1},{1,0},{0,-1},{-1,0}};
        for (int i = 0; i < 4; i++) {
            int newX = x + directions[i][0];
            int newY = y + directions[i][1];
            if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE && heatmap[newX][newY] >= 0) {
                heatmap[newX][newY] += 5;
            }
        }
    }
}

// Heatmap update on miss
void updateHeatmapOnMiss(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                         int numShips) {
    heatmap[x][y] = -999; // Mark as fired
}

// AI turn logic for hard difficulty
void aiTurn(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, aiTargetHard *aiTargetHard,
            int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips, int *radarUses) {
    int x = -1, y = -1;
    int shipDestroyed = 0;

    if (aiTargetHard->isHit) {
        if (!aiTargetHard->radarUsed && *radarUses < MAX_RADAR_USES) {
            int enemyFound = 0;
            RadarSweep(grid, 0, aiTargetHard->x, aiTargetHard->y, radarUses, NULL, 0, &enemyFound);
            printf("AI uses Radar Sweep at %c%d.\n", 'A' + aiTargetHard->y, aiTargetHard->x + 1);
            aiTargetHard->radarUsed = 1; // Mark that Radar has been used for this hit

            if (enemyFound) {
                // Use radar sweep information to determine case
                int southHit = 0, eastHit = 0;
                if (aiTargetHard->x + 1 < GRID_SIZE && grid[aiTargetHard->x + 1][aiTargetHard->y] != '~' &&
                    grid[aiTargetHard->x + 1][aiTargetHard->y] != 'h' && grid[aiTargetHard->x + 1][aiTargetHard->y] != 'm') {
                    southHit = 1;
                }
                if (aiTargetHard->y + 1 < GRID_SIZE && grid[aiTargetHard->x][aiTargetHard->y + 1] != '~' &&
                    grid[aiTargetHard->x][aiTargetHard->y + 1] != 'h' && grid[aiTargetHard->x][aiTargetHard->y + 1] != 'm') {
                    eastHit = 1;
                }

                aiTargetHard->referenceX = aiTargetHard->x;
                aiTargetHard->referenceY = aiTargetHard->y;

                if (southHit) {
                    aiTargetHard->caseNum = 1;
                    aiTargetHard->direction = 1; // South
                } else if (eastHit) {
                    aiTargetHard->caseNum = 2;
                    aiTargetHard->direction = 2; // East
                } else {
                    aiTargetHard->caseNum = 3;
                    aiTargetHard->direction = -1; // Will be set when we enter case 3
                    aiTargetHard->directionIndex = 0; // Start from the first direction
                }
            } else {
                // Radar did not detect ships; proceed with case 3 strategy in subsequent turns
                printf("AI did not detect any ships with radar. Will proceed with case 3 strategy.\n");

                aiTargetHard->caseNum = 3;
                aiTargetHard->direction = -1;
                aiTargetHard->directionIndex = 0;

                // Do not fire immediately; end the turn
                return; // End turn after using radar
            }
            return; // End turn after using radar
        }

        // Proceed with targeting logic
        if (aiTargetHard->caseNum == 1) {
            // Vertical targeting (South then North)
            x = aiTargetHard->referenceX + (aiTargetHard->direction == 1 ? 1 : -1);
            y = aiTargetHard->referenceY;

            if (x >= 0 && x < GRID_SIZE && grid[x][y] != 'm' && grid[x][y] != 'h') {
                // Use Artillery if available
                if (fleet->artilleryUnlocked && x < GRID_SIZE - 1 && y < GRID_SIZE - 1) {
                    Artillery(grid, fleet, x, y);
                    printf("AI uses Artillery at %c%d.\n", 'A' + y, x + 1);
                } else {
                    Fire(grid, fleet, x, y, &shipDestroyed);
                    printf("AI fires at %c%d.\n", 'A' + y, x + 1);
                }

                if (grid[x][y] == 'h') {
                    aiTargetHard->referenceX = x; // Update reference point
                    updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);

                    if (shipDestroyed) {
                        // Ship destroyed, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                } else {
                    if (aiTargetHard->direction == 1) {
                        // Switch direction to North
                        aiTargetHard->direction = 0;
                        aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit
                    } else {
                        // Both directions tried, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                    updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
                }
            } else {
                if (aiTargetHard->direction == 1) {
                    // Switch direction to North
                    aiTargetHard->direction = 0;
                    aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit
                } else {
                    // Both directions tried, reset AI targeting
                    initaiTargetHard(aiTargetHard);
                }
            }
            return; // End turn after firing once
        } else if (aiTargetHard->caseNum == 2) {
            // Horizontal targeting (East then West)
            x = aiTargetHard->referenceX;
            y = aiTargetHard->referenceY + (aiTargetHard->direction == 2 ? 1 : -1);

            if (y >= 0 && y < GRID_SIZE && grid[x][y] != 'm' && grid[x][y] != 'h') {
                // Use Artillery if available
                if (fleet->artilleryUnlocked && x < GRID_SIZE - 1 && y < GRID_SIZE - 1) {
                    Artillery(grid, fleet, x, y);
                    printf("AI uses Artillery at %c%d.\n", 'A' + y, x + 1);
                } else {
                    Fire(grid, fleet, x, y, &shipDestroyed);
                    printf("AI fires at %c%d.\n", 'A' + y, x + 1);
                }

                if (grid[x][y] == 'h') {
                    aiTargetHard->referenceY = y; // Update reference point
                    updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);

                    if (shipDestroyed) {
                        // Ship destroyed, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                } else {
                    if (aiTargetHard->direction == 2) {
                        // Switch direction to West
                        aiTargetHard->direction = 3;
                        aiTargetHard->referenceY = aiTargetHard->y; // Reset to initial hit
                    } else {
                        // Both directions tried, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                    updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
                }
            } else {
                if (aiTargetHard->direction == 2) {
                    // Switch direction to West
                    aiTargetHard->direction = 3;
                    aiTargetHard->referenceY = aiTargetHard->y; // Reset to initial hit
                } else {
                    // Both directions tried, reset AI targeting
                    initaiTargetHard(aiTargetHard);
                }
            }
            return; // End turn after firing once
        } else if (aiTargetHard->caseNum == 3) {
            // Targeting in all directions
            int directionSequence[4] = {0, 1, 3, 2}; // North, South, West, East

            if (aiTargetHard->directionIndex >= 4) {
                // All directions tried, reset AI targeting
                initaiTargetHard(aiTargetHard);
                return; // End turn
            }

            aiTargetHard->direction = directionSequence[aiTargetHard->directionIndex];
            x = aiTargetHard->referenceX;
            y = aiTargetHard->referenceY;

            // Determine next cell in current direction
            switch (aiTargetHard->direction) {
                case 0: // North
                    x = x - 1;
                    break;
                case 1: // South
                    x = x + 1;
                    break;
                case 2: // East
                    y = y + 1;
                    break;
                case 3: // West
                    y = y - 1;
                    break;
            }

            // Check if coordinates are valid and not already fired upon
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE &&
                grid[x][y] != 'm' && grid[x][y] != 'h') {
                // Use Artillery if available
                if (fleet->artilleryUnlocked && x < GRID_SIZE - 1 && y < GRID_SIZE - 1) {
                    Artillery(grid, fleet, x, y);
                    printf("AI uses Artillery at %c%d.\n", 'A' + y, x + 1);
                } else {
                    Fire(grid, fleet, x, y, &shipDestroyed);
                    printf("AI fires at %c%d.\n", 'A' + y, x + 1);
                }

                if (grid[x][y] == 'h') {
                    // Hit, update reference point and continue in same direction in next turn
                    aiTargetHard->referenceX = x;
                    aiTargetHard->referenceY = y;
                    updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);

                    if (shipDestroyed) {
                        // Ship destroyed, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    } else {
                        // Determine if the ship is vertical or horizontal based on direction
                        if (aiTargetHard->direction == 0 || aiTargetHard->direction == 1) {
                            aiTargetHard->caseNum = 1; // Vertical ship
                        } else {
                            aiTargetHard->caseNum = 2; // Horizontal ship
                        }
                    }
                } else {
                    // Miss, move to next direction
                    aiTargetHard->directionIndex++;
                    aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit position
                    aiTargetHard->referenceY = aiTargetHard->y;
                    updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
                }
            } else {
                // Invalid coordinate or already fired upon, move to next direction
                aiTargetHard->directionIndex++;
                aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit position
                aiTargetHard->referenceY = aiTargetHard->y;
            }
            return; // End AI's turn after one fire
        }
    } else {
        // AI has no current target, use heatmap to find the next target
        int maxHeat = -1;
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                if (heatmap[i][j] > maxHeat && grid[i][j] != 'm' && grid[i][j] != 'h') {
                    maxHeat = heatmap[i][j];
                    x = i;
                    y = j;
                }
            }
        }

        // Use Fire when selecting target from heatmap
        Fire(grid, fleet, x, y, &shipDestroyed);
        printf("AI fires at %c%d.\n", 'A' + y, x + 1);

        if (grid[x][y] == 'h') {
            aiTargetHard->isHit = 1;
            aiTargetHard->x = x;
            aiTargetHard->y = y;
            aiTargetHard->direction = -1;
            aiTargetHard->radarUsed = 0; // Reset radarUsed for new hit
            aiTargetHard->referenceX = x;
            aiTargetHard->referenceY = y;
            aiTargetHard->caseNum = 0;
            updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);
        } else {
            updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
        }
        return; // End AI's turn after firing once
    }
}

// Main function
int main() {
    char grid1[GRID_SIZE][GRID_SIZE];
    char grid2[GRID_SIZE][GRID_SIZE];
    char grid1for2[GRID_SIZE][GRID_SIZE];
    char grid2for1[GRID_SIZE][GRID_SIZE];

    Fleet fleet1, fleet2;
    SmokeScreen smokes1[MAX_SMOKE_SCREENS];
    SmokeScreen smokes2[MAX_SMOKE_SCREENS];
    int activeSmokeCount1 = 0, activeSmokeCount2 = 0;
    int radarUses1 = 0, radarUses2 = 0;

    initializeFleet(&fleet1);
    initializeFleet(&fleet2);

    initGrid(grid1);
    initGrid(grid2);
    initGrid(grid1for2);
    initGrid(grid2for1);

    // Place ships for both players
    placeShip(grid1, &fleet1.carrier, 0, 0, 0);
    placeShip(grid1, &fleet1.battleship, 2, 0, 1);
    placeShip(grid1, &fleet1.cruiser, 4, 4, 0);
    placeShip(grid1, &fleet1.submarine, 6, 6, 1);
    placeShip(grid1, &fleet1.destroyer, 8, 3, 0);

    placeShip(grid2, &fleet2.carrier, 1, 1, 0);
    placeShip(grid2, &fleet2.battleship, 3, 1, 1);
    placeShip(grid2, &fleet2.cruiser, 5, 5, 0);
    placeShip(grid2, &fleet2.submarine, 7, 7, 1);
    placeShip(grid2, &fleet2.destroyer, 9, 4, 0);

    aiTargetHard aiTargetHard1, aiTargetHard2;
    initaiTargetHard(&aiTargetHard1);
    initaiTargetHard(&aiTargetHard2);

    // Initialize heatmaps
    int shipSizes[] = {5, 4, 3, 3, 2};
    int numShips = 5;
    int heatmap1[GRID_SIZE][GRID_SIZE];
    int heatmap2[GRID_SIZE][GRID_SIZE];
    initHeatmap(heatmap1, shipSizes, numShips);
    initHeatmap(heatmap2, shipSizes, numShips);

    srand(time(NULL));
    int turn = rand() % 2 + 1;

    while (fleet1.shipsDestroyed != 5 && fleet2.shipsDestroyed != 5) {
        char input[32] = {0};
        char command[16] = {0};
        char target[4] = {0};
        int validCommand = 0;

        Fleet *opponentFleet = (turn == 1) ? &fleet2 : &fleet1;
        char (*oppositeGrid)[GRID_SIZE] = (turn == 1) ? grid2 : grid1;
        char (*playerGrid)[GRID_SIZE] = (turn == 1) ? grid1 : grid2;
        char (*opponentVisibleGrid)[GRID_SIZE] = (turn == 1) ? grid2for1 : grid1for2;
        SmokeScreen *activeSmokes = (turn == 1) ? smokes2 : smokes1;
        int *activeSmokeCount = (turn == 1) ? &activeSmokeCount2 : &activeSmokeCount1;
        int *radarUses = (turn == 1) ? &radarUses1 : &radarUses2;
        int (*heatmap)[GRID_SIZE] = (turn == 1) ? heatmap1 : heatmap2;
        aiTargetHard *aiTargetHard = (turn == 1) ? &aiTargetHard1 : &aiTargetHard2;

        displayGrids(turn, grid1, grid2, grid1for2, grid2for1);

        if (turn == 2) {
            aiTurn(oppositeGrid, opponentFleet, aiTargetHard, heatmap2, shipSizes, numShips, radarUses);
            UpdateSmokeScreens(activeSmokes, activeSmokeCount);
            turn = 1;
            continue;
        }

        while (!validCommand) {
            printf("Player %d, enter your command (e.g., Fire A5, Radar A5, Smoke A5): ", turn);
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0';

            if (sscanf(input, "%15s %3s", command, target) < 2) {
                printf("Invalid input format! Try again.\n");
                continue;
            }

            if (strcmp(command, "Fire") == 0 || strcmp(command, "Artillery") == 0) {
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0])) {
                    char col = toupper(target[0]);
                    int row;
                    if (strlen(target) == 2 && isdigit(target[1])) {
                        row = target[1] - '0';
                    } else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2])) {
                        row = (target[1] - '0') * 10 + (target[2] - '0');
                    } else {
                        printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                        continue;
                    }

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        validCommand = 1;
                        int shipDestroyed;
                        if (strcmp(command, "Fire") == 0) {
                            Fire(oppositeGrid, opponentFleet, row - 1, col - 'A', &shipDestroyed);
                        } else if (opponentFleet->artilleryUnlocked) {
                            Artillery(oppositeGrid, opponentFleet, row - 1, col - 'A');
                        } else {
                            printf("Artillery is not unlocked yet!\n");
                            validCommand = 0; // Do not end turn if command is invalid
                        }
                    } else {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                } else {
                    printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                }
            } else if (strcmp(command, "Torpedo") == 0) {
                if (isalpha(target[0]) && strlen(target) == 1) {
                    char col = toupper(target[0]);
                    if (col >= 'A' && col <= 'J') {
                        validCommand = 1;
                        Torpedo(oppositeGrid, opponentFleet, col - 'A', 'C'); // 'C' for column
                    } else {
                        printf("Invalid column! Use A-J for columns.\n");
                    }
                } else if (isdigit(target[0])) {
                    int row = atoi(target);
                    if (row >= 1 && row <= 10) {
                        validCommand = 1;
                        Torpedo(oppositeGrid, opponentFleet, row - 1, 'R'); // 'R' for row
                    } else {
                        printf("Invalid row! Use 1-10 for rows.\n");
                    }
                } else {
                    printf("Invalid target for Torpedo! Use a column (A-J) or row (1-10).\n");
                }
            } else if (strcmp(command, "Radar") == 0) {
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0])) {
                    char col = toupper(target[0]);
                    int row;
                    if (strlen(target) == 2 && isdigit(target[1])) {
                        row = target[1] - '0';
                    } else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2])) {
                        row = (target[1] - '0') * 10 + (target[2] - '0');
                    } else {
                        printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                        continue;
                    }

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        int enemyFound;
                        RadarSweep(oppositeGrid, 0, row - 1, col - 'A', radarUses, activeSmokes,
                                   *activeSmokeCount, &enemyFound);
                        validCommand = 1; // End turn after using Radar
                        printf("Radar sweep performed. Turn ends.\n");
                    } else {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                } else {
                    printf("Invalid target! Use a format like 'A5'.\n");
                }
            } else if (strcmp(command, "Smoke") == 0) {
                if (strlen(target) == 2 && isalpha(target[0]) && isdigit(target[1])) {
                    char col = toupper(target[0]);
                    int row = target[1] - '0';

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        validCommand = 1;
                        DeploySmokeScreen(activeSmokes, activeSmokeCount, row - 1, col - 'A');
                    } else {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                } else {
                    printf("Invalid target! Use a format like 'A5'.\n");
                }
            } else {
                printf("Unknown command '%s'. Available commands: Fire, Artillery, Radar, Smoke.\n", command);
            }
        }

        UpdateSmokeScreens(activeSmokes, activeSmokeCount);
        turn = (turn == 1) ? 2 : 1;
    }

    if (fleet1.shipsDestroyed == 5) {
        printf("Player 2 wins!\n");
    } else {
        printf("Player 1 wins!\n");
    }

    return 0;
}
