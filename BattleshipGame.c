#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define GRID_SIZE 10
#define MAX_RADAR_USES 3
#define MAX_SMOKE_SCREENS 5
#define DEBUG_MODE 0 // Set to 1 to enable debug outputs

// Ship structure
typedef struct
{
    char type;
    int size;
    int hits;
} Ship;

// Fleet structure
typedef struct
{
    Ship carrier;
    Ship battleship;
    Ship cruiser;
    Ship submarine;
    Ship destroyer;
    int shipsDestroyed;
    int artilleryUnlocked;
    int torpedoUnlocked;
    int radarUses;

    // Sunk ship flags
    int carrierSunkReported;
    int battleshipSunkReported;
    int cruiserSunkReported;
    int submarineSunkReported;
    int destroyerSunkReported;
} Fleet;

// Smoke screen structure
typedef struct
{
    int x;
    int y;
    int turnsRemaining;
} SmokeScreen;

// Radar target structure
typedef struct
{
    int x;
    int y;
    int active;
} RadarTarget;

// AI target structure for Medium AI
typedef struct
{
    int state;
    int hits[5][2];
    int hitCount;
    int isSinking;
    int orientation; // 0 = Unknown, 1 = Horizontal, 2 = Vertical
    int direction;
    int usedArtillery;
    int shipLost;
    RadarTarget radarTargets[4];
    int radarTargetCount;
    int radarIndex;
    int turnsSinceLastHit;
    int initialRadarUsed;

    int searchX;
    int searchY;
} AITargetMedium;

// AI states
#define STATE_SCANNING 0
#define STATE_TARGETING 1
#define STATE_SINKING 2
#define STATE_SEARCHING 3

// Ship types
typedef enum
{
    SHIP_NONE,
    SHIP_CARRIER,
    SHIP_BATTLESHIP,
    SHIP_CRUISER,
    SHIP_SUBMARINE,
    SHIP_DESTROYER
} ShipType;

// Function prototypes
void initGrid(char grid[GRID_SIZE][GRID_SIZE]);
void printGrid(char grid[GRID_SIZE][GRID_SIZE]);
void displayGrids(int turn, char p1Grid[GRID_SIZE][GRID_SIZE],
                  char p2Grid[GRID_SIZE][GRID_SIZE],
                  char p1Visible[GRID_SIZE][GRID_SIZE],
                  char p2Visible[GRID_SIZE][GRID_SIZE], int gameMode);
void initializeFleet(Fleet *fleet);
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y,
              int orientation);
int Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y,
         void *aiTarget, int showMessage);
void Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y,
               void *aiTarget, int showMessage);
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int index,
             char axis, void *aiTarget, int showMessage);
int RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int x, int y, int *radarUses,
               SmokeScreen smokes[], int activeSmokeCount, AITargetMedium *aiTarget);
int PlayerRadarSweep(char grid[GRID_SIZE][GRID_SIZE], int x, int y, int *radarUses,
                     SmokeScreen smokes[], int activeSmokeCount);
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y, Fleet *fleet);
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount);
void initAITargetMedium(AITargetMedium *aiTarget);
void aiTurnMedium(char grid[][GRID_SIZE], Fleet *opponentFleet, Fleet *aiFleet,
                  AITargetMedium *aiTarget, SmokeScreen smokes[],
                  int *activeSmokeCount, int playerRadarUses);
int shouldUseRadar(AITargetMedium *aiTarget, Fleet *fleet);
int shouldDeploySmoke(AITargetMedium *aiTarget, Fleet *fleet, int playerRadarUses, int activeSmokeCount);
int shouldUseTorpedo(AITargetMedium *aiTarget, Fleet *fleet);
int shouldUseArtillery(AITargetMedium *aiTarget, Fleet *fleet);
void selectRadarTarget(char grid[GRID_SIZE][GRID_SIZE], int *targetX,
                       int *targetY);
void selectSmokeTarget(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet,
                       int *targetX, int *targetY);
int selectTorpedoTargetColumn(char grid[GRID_SIZE][GRID_SIZE],
                              AITargetMedium *aiTarget);
void selectArtilleryTarget(char grid[GRID_SIZE][GRID_SIZE],
                           AITargetMedium *aiTarget, int *targetX,
                           int *targetY);
void aiHuntMode(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet,
                AITargetMedium *aiTarget);
int isCellFeasible(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x,
                   int y);
void determineOrientation(AITargetMedium *aiTarget);
int areaAlreadyScanned(char grid[GRID_SIZE][GRID_SIZE], int x, int y);
int isGameOver(Fleet *fleet);
void getAdjacentTargets(char grid[GRID_SIZE][GRID_SIZE], int x, int y, AITargetMedium *aiTarget, int orientation);
void aiTargetMode(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet,
                  AITargetMedium *aiTarget, Fleet *aiFleet);
void aiSearchMode(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet,
                  AITargetMedium *aiTarget, Fleet *aiFleet);
void aiSinkingMode(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet,
                   AITargetMedium *aiTarget, Fleet *aiFleet);
ShipType isShipSunk(Fleet *fleet);
char *getShipName(ShipType shipType);
void aiRecordMiss(int x, int y, int columnMisses[GRID_SIZE]);

// Global variable for AI difficulty
int aiDifficulty = 1; // 1 = Easy, 2 = Medium, 3 = Hard

// Initializes the grid with water '~'
void initGrid(char grid[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            grid[i][j] = '~';
}

// Prints the grid with column and row headers
void printGrid(char grid[GRID_SIZE][GRID_SIZE])
{
    printf("   A B C D E F G H I J\n");
    for (int i = 0; i < GRID_SIZE; i++)
    {
        printf("%2d ", i + 1); // Row numbers
        for (int j = 0; j < GRID_SIZE; j++)
        {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
}

// Displays the player's grid and their view of the opponent's grid
void displayGrids(int turn, char p1Grid[GRID_SIZE][GRID_SIZE],
                  char p2Grid[GRID_SIZE][GRID_SIZE],
                  char p1Visible[GRID_SIZE][GRID_SIZE],
                  char p2Visible[GRID_SIZE][GRID_SIZE], int gameMode)
{
    if (gameMode == 1)
    { // Player vs Player
        if (turn == 1)
        {
            printf("\nPlayer 1's Grid:\n");
            printGrid(p1Grid);
            printf("\nPlayer 1's View of Player 2's Grid:\n");
            printGrid(p1Visible);
        }
        else
        {
            printf("\nPlayer 2's Grid:\n");
            printGrid(p2Grid);
            printf("\nPlayer 2's View of Player 1's Grid:\n");
            printGrid(p2Visible);
        }
    }
    else
    { // Player vs AI
        printf("\nYour Grid:\n");
        printGrid(p1Grid);
        printf("\nOpponent's Visible Grid:\n");
        printGrid(p1Visible);
    }
}

// Initializes the fleet with default ships and resets sunk flags
void initializeFleet(Fleet *fleet)
{
    fleet->carrier = (Ship){'C', 5, 0};
    fleet->battleship = (Ship){'B', 4, 0};
    fleet->cruiser = (Ship){'c', 3, 0};
    fleet->submarine = (Ship){'s', 2, 0};
    fleet->destroyer = (Ship){'d', 2, 0};
    fleet->shipsDestroyed = 0;
    fleet->artilleryUnlocked = 0;
    fleet->torpedoUnlocked = 0;
    fleet->radarUses = 0;

    // Initialize sunk ship flags
    fleet->carrierSunkReported = 0;
    fleet->battleshipSunkReported = 0;
    fleet->cruiserSunkReported = 0;
    fleet->submarineSunkReported = 0;
    fleet->destroyerSunkReported = 0;
}

// Places a ship on the grid
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y,
              int orientation)
{
    // Check if ship can be placed
    for (int i = 0; i < ship->size; i++)
    {
        int xi = x + (orientation == 1 ? 0 : i);
        int yi = y + (orientation == 1 ? i : 0);
        if (xi >= GRID_SIZE || yi >= GRID_SIZE || grid[xi][yi] != '~')
            return -1; // Cannot place ship here
    }
    // Place the ship
    for (int i = 0; i < ship->size; i++)
    {
        int xi = x + (orientation == 1 ? 0 : i);
        int yi = y + (orientation == 1 ? i : 0);
        grid[xi][yi] = ship->type;
    }
    return 0; // Success
}

// Fires at a specific location
int Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y,
         void *aiTarget, int showMessage)
{
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE)
    {
        if (showMessage)
            printf("Invalid coordinates!\n");
        return -1;
    }

    if (grid[x][y] == '~')
    {
        grid[x][y] = 'm'; // Miss
        if (showMessage)
            printf("Missed at %c%d!\n", 'A' + y, x + 1);
        return 0;
    }
    else if (grid[x][y] != 'm' && grid[x][y] != 'h')
    {
        char shipType = grid[x][y];
        grid[x][y] = 'h'; // Hit
        if (showMessage)
            printf("Hit at %c%d!\n", 'A' + y, x + 1);

        // Update the corresponding ship's hit count
        Ship *hitShip = NULL;
       switch (tolower(shipType))
{
    case 'c':
        hitShip = &fleet->carrier;
        break;
    case 'b':
        hitShip = &fleet->battleship;
        break;
    case 's':
        hitShip = &fleet->submarine;
        break;
    case 'd':
        hitShip = &fleet->destroyer;
        break;
    default:
        break;
}


        if (hitShip)
        {
            hitShip->hits++;
            if (hitShip->hits == hitShip->size)
            {
                printf("You have sunk the %s!\n",
                       (shipType == 'C')   ? "Carrier"
                       : (shipType == 'B') ? "Battleship"
                       : (shipType == 'c') ? "Cruiser"
                       : (shipType == 's') ? "Submarine"
                       : (shipType == 'd') ? "Destroyer"
                                           : "Unknown");

                fleet->shipsDestroyed++;

                // Unlock abilities based on shipsDestroyed
                if (!fleet->artilleryUnlocked && fleet->shipsDestroyed >= 1)
                {
                    fleet->artilleryUnlocked = 1;
                    if (showMessage)
                        printf("Artillery has been unlocked!\n");
                }

                if (!fleet->torpedoUnlocked && fleet->shipsDestroyed >= 3)
                {
                    fleet->torpedoUnlocked = 1;
                    if (showMessage)
                        printf("Torpedo has been unlocked!\n");
                }
            }
            return 1;
        }
    }

    if (grid[x][y] == 'h' || grid[x][y] == 'm')
    {
        if (showMessage)
            printf("Already targeted this cell at %c%d!\n", 'A' + y, x + 1);
        return -1;
    }

    return -1;
}

// Executes an artillery strike
void Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y,
               void *aiTarget, int showMessage)
{
    if (fleet->artilleryUnlocked)
    {
        if (showMessage)
            printf("Artillery attack at %c%d!\n", 'A' + y, x + 1);
        for (int i = x; i < x + 2 && i < GRID_SIZE; i++)
        {
            for (int j = y; j < y + 2 && j < GRID_SIZE; j++)
            {
                Fire(grid, fleet, i, j, aiTarget, showMessage);
            }
        }
        fleet->artilleryUnlocked = 0; // Reset artillery availability
    }
    else
    {
        if (showMessage)
            printf("Artillery is not unlocked yet!\n");
    }
}

// Executes a torpedo strike on a column or row
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int index,
             char axis, void *aiTarget, int showMessage)
{
    if (fleet->torpedoUnlocked)
    {
        if (axis == 'C')
        {
            if (showMessage)
                printf("Torpedo attack on column %c!\n", 'A' + index);
            for (int i = 0; i < GRID_SIZE; i++)
            {
                Fire(grid, fleet, i, index, aiTarget, showMessage);
            }
        }
        else if (axis == 'R')
        {
            if (showMessage)
                printf("Torpedo attack on row %d!\n", index + 1);
            for (int j = 0; j < GRID_SIZE; j++)
            {
                Fire(grid, fleet, index, j, aiTarget, showMessage);
            }
        }
        fleet->torpedoUnlocked = 0; // Reset torpedo availability
    }
    else
    {
        if (showMessage)
            printf("Torpedo is not unlocked yet!\n");
    }
}

// Performs radar sweep for AI and stores detected targets
int RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int x, int y, int *radarUses,
               SmokeScreen smokes[], int activeSmokeCount, AITargetMedium *aiTarget)
{
    // Check if radar is blocked by smoke
    for (int i = 0; i < activeSmokeCount; i++)
    {
        if (x >= smokes[i].x && x < smokes[i].x + 2 &&
            y >= smokes[i].y && y < smokes[i].y + 2)
        {
            printf("Radar sweep at %c%d is blocked by smoke.\n", 'A' + y, x + 1);
            return 0;
        }
    }

    if (*radarUses >= MAX_RADAR_USES)
    {
        printf("No more radar sweeps available.\n");
        return 0;
    }

    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1)
    {
        printf("Invalid coordinates for radar sweep.\n");
        return 0;
    }

    int enemyFound = 0;
    aiTarget->radarTargetCount = 0;
    aiTarget->radarIndex = 0; // Reset radar index

    if (DEBUG_MODE)
        printf("AI Radar Sweep Area at %c%d:\n", 'A' + y, x + 1);
    for (int i = x; i < x + 2 && i < GRID_SIZE; i++)
    {
        for (int j = y; j < y + 2 && j < GRID_SIZE; j++)
        {
            if (DEBUG_MODE)
                printf("Scanning %c%d: %c\n", 'A' + j, i + 1, grid[i][j]);
            if (grid[i][j] != '~' && grid[i][j] != 'h' && grid[i][j] != 'm')
            {
                // Store the radar target if not already targeted
                int alreadyTargeted = 0;
                for (int k = 0; k < aiTarget->radarTargetCount; k++)
                {
                    if (aiTarget->radarTargets[k].x == i && aiTarget->radarTargets[k].y == j)
                    {
                        alreadyTargeted = 1;
                        break;
                    }
                }

                if (!alreadyTargeted && aiTarget->radarTargetCount < 4)
                {
                    aiTarget->radarTargets[aiTarget->radarTargetCount].x = i;
                    aiTarget->radarTargets[aiTarget->radarTargetCount].y = j;
                    aiTarget->radarTargets[aiTarget->radarTargetCount].active = 1;
                    aiTarget->radarTargetCount++;
                    enemyFound = 1;
                }
            }
        }
    }

    (*radarUses)++;
    if (enemyFound)
    {
        printf("Enemy ships found in radar sweep area.\n");
    }

    return enemyFound;
}

// Player Radar Sweep (similar to AI's RadarSweep but without AI targeting)
int PlayerRadarSweep(char grid[GRID_SIZE][GRID_SIZE], int x, int y, int *radarUses,
                     SmokeScreen smokes[], int activeSmokeCount)
{
    // Check if radar is blocked by smoke
    for (int i = 0; i < activeSmokeCount; i++)
    {
        if (x >= smokes[i].x && x < smokes[i].x + 2 &&
            y >= smokes[i].y && y < smokes[i].y + 2)
        {
            printf("Radar sweep at %c%d is blocked by smoke.\n", 'A' + y, x + 1);
            return 0;
        }
    }

    if (*radarUses >= MAX_RADAR_USES)
    {
        printf("No more radar sweeps available.\n");
        return 0;
    }

    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1)
    {
        printf("Invalid coordinates for radar sweep.\n");
        return 0;
    }

    int enemyFound = 0;
    for (int i = x; i < x + 2 && i < GRID_SIZE; i++)
    {
        for (int j = y; j < y + 2 && j < GRID_SIZE; j++)
        {
            if (grid[i][j] != '~' && grid[i][j] != 'h' && grid[i][j] != 'm')
            {
                enemyFound = 1;
            }
        }
    }

    (*radarUses)++;
    if (enemyFound)
    {
        printf("Radar sweep detected enemy ships in the area.\n");
    }
    else
    {
        printf("Radar sweep found no enemy ships in the area.\n");
    }

    return enemyFound;
}

// Deploys a smoke screen
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y, Fleet *fleet)
{
    // Check if smoke screens are unlocked
    int allowedSmokes = fleet->shipsDestroyed; // Allowed smokes based on sunk ships

    // Check if the AI has exceeded their allowed smoke screens
    if (*activeSmokeCount >= allowedSmokes)
    {
        printf("No more smoke screens available.\n");
        return; // AI loses their turn
    }

    // Validate coordinates
    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1)
    {
        printf("Invalid coordinates for smoke screen deployment.\n");
        return;
    }

    // Deploy the smoke screen
    smokes[*activeSmokeCount].x = x;
    smokes[*activeSmokeCount].y = y;
    smokes[*activeSmokeCount].turnsRemaining = 3;
    (*activeSmokeCount)++;

    printf("Smoke screen deployed at %c%d.\n", 'A' + y, x + 1);
}

// Updates smoke screens by decrementing their remaining turns
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount)
{
    for (int i = 0; i < *activeSmokeCount; i++)
    {
        smokes[i].turnsRemaining--;
        if (smokes[i].turnsRemaining <= 0)
        {
            if (DEBUG_MODE)
                printf("Smoke screen at %c%d has expired.\n", 'A' + smokes[i].y,
                       smokes[i].x + 1);
            // Remove the smoke screen by shifting the array
            for (int j = i; j < *activeSmokeCount - 1; j++)
            {
                smokes[j] = smokes[j + 1];
            }
            (*activeSmokeCount)--;
            i--; // Adjust index after removal
        }
    }
}

// Initializes AI target for Medium AI
void initAITargetMedium(AITargetMedium *aiTarget)
{
    aiTarget->hitCount = 0;
    aiTarget->isSinking = 0;
    aiTarget->orientation = 0;
    aiTarget->direction = 1;
    aiTarget->usedArtillery = 0;
    aiTarget->shipLost = 0;
    aiTarget->radarTargetCount = 0;
    aiTarget->radarIndex = 0;
    aiTarget->turnsSinceLastHit = 0;
    aiTarget->initialRadarUsed = 0;

    // Initialize state to STATE_SCANNING to perform radar sweep first
    aiTarget->state = STATE_SCANNING;

    // Reset search positions
    aiTarget->searchX = rand() % GRID_SIZE;
    aiTarget->searchY = rand() % GRID_SIZE;

    // Clear radar targets and hits
    memset(aiTarget->radarTargets, 0, sizeof(aiTarget->radarTargets));
    memset(aiTarget->hits, 0, sizeof(aiTarget->hits));
}

// AI turn logic for Medium AI
void aiTurnMedium(char grid[][GRID_SIZE], Fleet *opponentFleet, Fleet *aiFleet,
                  AITargetMedium *aiTarget, SmokeScreen smokes[],
                  int *activeSmokeCount, int playerRadarUses)
{
    printf("\n--- AI's Turn (Medium) ---\n");
    // Update smoke screens
    UpdateSmokeScreens(smokes, activeSmokeCount);

    // Increment turns since last hit
    aiTarget->turnsSinceLastHit++;

    // Check if AI should deploy smoke
    if (shouldDeploySmoke(aiTarget, aiFleet, playerRadarUses, *activeSmokeCount))
    {
        int targetX, targetY;
        selectSmokeTarget(grid, aiFleet, &targetX, &targetY);
        printf("AI deploys Smoke Screen at %c%d.\n", 'A' + targetY, targetX + 1);
        DeploySmokeScreen(smokes, activeSmokeCount, targetX, targetY, aiFleet);
        return; // Smoke deployment counts as the turn
    }

    // Check if AI should use radar
    if (shouldUseRadar(aiTarget, aiFleet)) {
        int targetX, targetY;
        selectRadarTarget(grid, &targetX, &targetY);
        if (targetX != -1 && targetY != -1) {
            printf("AI performs Radar Sweep at %c%d!\n", 'A' + targetY, targetX + 1);
            int radarResult = RadarSweep(grid, targetX, targetY, &aiFleet->radarUses, smokes, *activeSmokeCount, aiTarget);
            if (radarResult) {
                printf("AI detects enemy ships in the radar sweep area.\n");
                aiTarget->state = STATE_TARGETING;
                // Reset turns since last hit
                aiTarget->turnsSinceLastHit = 0;
            } else {
                printf("AI finds no enemy ships in the radar sweep area.\n");
                aiTarget->state = STATE_SEARCHING;
            }
            aiTarget->initialRadarUsed = 1; // Mark radar as used
            return; // Radar usage counts as the turn
        }
    }

    // AI state machine
    switch (aiTarget->state) {
    case STATE_SCANNING:
        // Perform initial radar sweep
        if (shouldUseRadar(aiTarget, aiFleet)) {
            int targetX, targetY;
            selectRadarTarget(grid, &targetX, &targetY);
            if (targetX != -1 && targetY != -1) {
                printf("AI performs Radar Sweep at %c%d!\n", 'A' + targetY, targetX + 1);
                int radarResult = RadarSweep(grid, targetX, targetY, &aiFleet->radarUses, smokes, *activeSmokeCount, aiTarget);
                if (radarResult) {
                    printf("AI detects enemy ships in the radar sweep area.\n");
                    aiTarget->state = STATE_TARGETING;
                    // Reset turns since last hit
                    aiTarget->turnsSinceLastHit = 0;
                } else {
                    printf("AI finds no enemy ships in the radar sweep area.\n");
                    aiTarget->state = STATE_SEARCHING;
                }
                aiTarget->initialRadarUsed = 1; // Mark radar as used
                return; // Radar usage counts as the turn
            }
        } else {
            aiTarget->state = STATE_SEARCHING;
        }
        break;
    case STATE_TARGETING:
        aiTargetMode(grid, opponentFleet, aiTarget, aiFleet);
        break;
    case STATE_SINKING:
        aiSinkingMode(grid, opponentFleet, aiTarget, aiFleet);
        break;
    case STATE_SEARCHING:
        aiSearchMode(grid, opponentFleet, aiTarget, aiFleet);
        break;
    default:
        aiTarget->state = STATE_SEARCHING;
        break;
    }
}


// Decision-making functions

int shouldUseRadar(AITargetMedium *aiTarget, Fleet *fleet) {
    // Use radar sweep on the first turn if not used yet
    if (!aiTarget->initialRadarUsed && fleet->radarUses < MAX_RADAR_USES) {
        return 1;
    }
    // Use radar if we have uses left and haven't hit for 5 turns
    if (fleet->radarUses < MAX_RADAR_USES && aiTarget->turnsSinceLastHit >= 5) {
        return 1;
    }
    return 0;
}

int shouldDeploySmoke(AITargetMedium *aiTarget, Fleet *fleet, int playerRadarUses, int activeSmokeCount)
{
    // Deploy smoke if a ship is lost
    if (aiTarget->shipLost)
    {
        aiTarget->shipLost = 0;
        return 1;
    }
    // Deploy smoke if player used radar recently and have smoke screens left
    if (playerRadarUses > 0 && activeSmokeCount < fleet->shipsDestroyed)
    {
        return 1;
    }
    return 0;
}

int shouldUseTorpedo(AITargetMedium *aiTarget, Fleet *fleet) {
    // Use torpedo if not sinking and torpedo is unlocked
    if (!aiTarget->isSinking && fleet->torpedoUnlocked) {
        return 1;
    }
    return 0;
}

int shouldUseArtillery(AITargetMedium *aiTarget, Fleet *fleet)
{
    // Use artillery if unlocked and not used this turn
    if (fleet->artilleryUnlocked && !aiTarget->usedArtillery && aiTarget->isSinking)
    {
        return 1;
    }
    return 0;
}

// Selection functions

void selectRadarTarget(char grid[GRID_SIZE][GRID_SIZE], int *targetX,
                       int *targetY)
{
    // Randomly select a valid radar target
    do
    {
        *targetX = rand() % (GRID_SIZE - 1);
        *targetY = rand() % (GRID_SIZE - 1);
    } while (areaAlreadyScanned(grid, *targetX, *targetY));
}

void selectSmokeTarget(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet,
                       int *targetX, int *targetY)
{
    // Randomly select a valid smoke target
    *targetX = rand() % (GRID_SIZE - 1);
    *targetY = rand() % (GRID_SIZE - 1);
}

int selectTorpedoTargetColumn(char grid[GRID_SIZE][GRID_SIZE],
                              AITargetMedium *aiTarget)
{
    // Select the column with the least misses
    int minMisses = GRID_SIZE + 1;
    int targetColumn = 0;
    for (int i = 0; i < GRID_SIZE; i++)
    {
        int misses = 0;
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (grid[j][i] == 'm')
                misses++;
        }
        if (misses < minMisses)
        {
            minMisses = misses;
            targetColumn = i;
        }
    }
    return targetColumn;
}

void selectArtilleryTarget(char grid[GRID_SIZE][GRID_SIZE],
                           AITargetMedium *aiTarget, int *targetX,
                           int *targetY)
{
    int x = aiTarget->hits[aiTarget->hitCount - 1][0];
    int y = aiTarget->hits[aiTarget->hitCount - 1][1];
    if (x > GRID_SIZE - 2)
        x = GRID_SIZE - 2;
    if (y > GRID_SIZE - 2)
        y = GRID_SIZE - 2;
    *targetX = x;
    *targetY = y;
}

// AI behavior functions

void aiTargetMode(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet,
                  AITargetMedium *aiTarget, Fleet *aiFleet)
{
    if (DEBUG_MODE)
    {
        printf("DEBUG: Processing Targeting Mode.\n");
    }

    // Process all radar targets first
    for (int i = 0; i < aiTarget->radarTargetCount; i++)
    {
        if (aiTarget->radarTargets[i].active)
        {
            int x = aiTarget->radarTargets[i].x;
            int y = aiTarget->radarTargets[i].y;
            printf("AI targets radar detected cell %c%d.\n", 'A' + y, x + 1);
            int result = Fire(grid, opponentFleet, x, y, aiTarget, 0);
            if (result == 1)
            {
                printf("AI hits at %c%d.\n", 'A' + y, x + 1);
                aiTarget->hits[aiTarget->hitCount][0] = x;
                aiTarget->hits[aiTarget->hitCount][1] = y;
                aiTarget->hitCount++;
                aiTarget->radarTargets[i].active = 0; // Mark as processed

                // Reset turns since last hit
                aiTarget->turnsSinceLastHit = 0;

                if (isShipSunk(opponentFleet) != SHIP_NONE)
                {
                    printf("AI has sunk a ship!\n");
                    aiTarget->state = STATE_SEARCHING;
                    aiTarget->hitCount = 0;
                    aiTarget->orientation = 0;
                    aiTarget->direction = 1;
                    aiTarget->radarTargetCount = 0; // Reset radar targets
                }
                else
                {
                    aiTarget->state = STATE_SINKING;
                }
                return;
            }
            else
            {
                printf("AI misses at %c%d.\n", 'A' + y, x + 1);
                aiTarget->radarTargets[i].active = 0; // Mark as processed
            }
            // Update turns since last hit
            aiTarget->turnsSinceLastHit++;
            return; // Exit after one action
        }
    }

    // If no radar targets left, proceed to Sinking Mode if there are hits
    if (aiTarget->hitCount > 0)
    {
        aiTarget->state = STATE_SINKING;
    }
    else
    {
        aiTarget->state = STATE_SEARCHING;
    }
}


void aiSearchMode(char grid[][GRID_SIZE], Fleet *opponentFleet,
                  AITargetMedium *aiTarget, Fleet *aiFleet)
{
    if (DEBUG_MODE)
    {
        printf("DEBUG: Entering aiSearchMode.\n");
        printf("DEBUG: Current Search Coordinates - X: %d, Y: %d\n",
               aiTarget->searchX, aiTarget->searchY);
    }

    int attempts = 0;
    int maxAttempts = GRID_SIZE * GRID_SIZE;

    int x = aiTarget->searchX;
    int y = aiTarget->searchY;

    while (attempts < maxAttempts)
    {
        // Check if the current cell is on the checkerboard pattern
        if ((x + y) % 2 == 0 && grid[x][y] == '~')
        {
            printf("AI fires at %c%d (Checkerboard Search).\n", 'A' + y, x + 1);
            int result = Fire(grid, opponentFleet, x, y, aiTarget, 0);

            if (result == 1)
            {
                printf("AI hits at %c%d.\n", 'A' + y, x + 1);
                // Record the hit
                aiTarget->hits[0][0] = x;
                aiTarget->hits[0][1] = y;
                aiTarget->hitCount = 1;
                aiTarget->state = STATE_TARGETING;
                // Reset turns since last hit
                aiTarget->turnsSinceLastHit = 0;
            }
            else
            {
                printf("AI misses at %c%d.\n", 'A' + y, x + 1);
            }

            // Update search coordinates for next turn
            y += 2;

            if (y >= GRID_SIZE)
            {
                // Move to the next row and adjust column based on row parity
                y = (x % 2 == 0) ? 1 : 0;
                x = (x + 1) % GRID_SIZE;
            }

            aiTarget->searchX = x;
            aiTarget->searchY = y;
            return; // Exit after one fire
        }
        else
        {
            // Move to the next cell in the checkerboard pattern
            y += 2;

            if (y >= GRID_SIZE)
            {
                // Move to the next row and adjust column based on row parity
                y = (x % 2 == 0) ? 1 : 0;
                x = (x + 1) % GRID_SIZE;
            }

            aiTarget->searchX = x;
            aiTarget->searchY = y;
        }

        attempts++;
    }

    // If all cells have been targeted
    printf("AI has exhausted all targeting options.\n");
    aiTarget->state = STATE_SEARCHING;

    // Reset search coordinates
    aiTarget->searchX = rand() % GRID_SIZE;
    aiTarget->searchY = rand() % 2; // Start at 0 or 1 to maintain the pattern
}




void aiSinkingMode(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet,
                   AITargetMedium *aiTarget, Fleet *aiFleet)
{
    if (DEBUG_MODE)
        printf("AI is in Sinking Mode.\n");

    if (aiTarget->hitCount < 1)
    {
        aiTarget->state = STATE_TARGETING;
        return;
    }

    // Determine orientation if possible
    if (aiTarget->hitCount == 1)
    {
        // Add all adjacent cells as targets (orientation unknown)
        int x = aiTarget->hits[0][0];
        int y = aiTarget->hits[0][1];
        getAdjacentTargets(grid, x, y, aiTarget, aiTarget->orientation);
    }
    else
    {
        // Determine orientation based on hits
        determineOrientation(aiTarget);
        // Re-add adjacent targets based on the determined orientation
        // Clear existing radarTargets and add only aligned targets
        aiTarget->radarTargetCount = 0;
        for (int i = 0; i < aiTarget->hitCount; i++)
        {
            int x = aiTarget->hits[i][0];
            int y = aiTarget->hits[i][1];
            getAdjacentTargets(grid, x, y, aiTarget, aiTarget->orientation);
        }
    }

    // Check if AI should use artillery
    if (shouldUseArtillery(aiTarget, aiFleet))
    {
        int targetX, targetY;
        selectArtilleryTarget(grid, aiTarget, &targetX, &targetY);
        printf("AI uses Artillery at %c%d.\n", 'A' + targetY, targetX + 1);
        Artillery(grid, opponentFleet, targetX, targetY, aiTarget, 0);
        aiTarget->usedArtillery = 1; // Mark artillery as used
        return;
    }

    if (shouldUseTorpedo(aiTarget, aiFleet))
    {
        int targetIndex = selectTorpedoTargetColumn(grid, aiTarget);
        printf("AI uses Torpedo on column %c.\n", 'A' + targetIndex);
        Torpedo(grid, opponentFleet, targetIndex, 'C', aiTarget, 0);
        aiFleet->torpedoUnlocked = 0; // Reset torpedo availability
        return;                       // Torpedo usage counts as the turn
    }

    // Target the first active sinking cell aligned with orientation
    for (int i = 0; i < aiTarget->radarTargetCount; i++)
    {
        RadarTarget *currentTarget = &aiTarget->radarTargets[i];
        if (currentTarget->active)
        {
            int x = currentTarget->x;
            int y = currentTarget->y;

            printf("AI targets sinking mode cell %c%d.\n", 'A' + y, x + 1);
            int result = Fire(grid, opponentFleet, x, y, aiTarget, 0);

            if (result == 1)
            {
                printf("AI hits at %c%d.\n", 'A' + y, x + 1);
                aiTarget->hits[aiTarget->hitCount][0] = x;
                aiTarget->hits[aiTarget->hitCount][1] = y;
                aiTarget->hitCount++;

                // Reset turns since last hit
                aiTarget->turnsSinceLastHit = 0;

                // Add adjacent targets based on orientation
                if (aiTarget->orientation != 0)
                {
                    getAdjacentTargets(grid, x, y, aiTarget, aiTarget->orientation);
                }

                // Check if a ship is sunk and identify which one
                ShipType sunkShip = isShipSunk(opponentFleet);
                if (sunkShip != SHIP_NONE)
                {
                    printf("AI has sunk your %s!\n", getShipName(sunkShip));

                    // Reset AI targeting variables
                    aiTarget->state = STATE_SEARCHING;
                    aiTarget->hitCount = 0;
                    aiTarget->orientation = 0;
                    aiTarget->direction = 1;
                    aiTarget->radarTargetCount = 0;
                    memset(aiTarget->hits, 0, sizeof(aiTarget->hits));
                    aiTarget->usedArtillery = 0; // Reset artillery usage

                    // Unlock abilities if applicable
                    aiFleet->shipsDestroyed++;
                    if (aiFleet->shipsDestroyed == 1 && !aiFleet->artilleryUnlocked)
                    {
                        aiFleet->artilleryUnlocked = 1;
                        printf("AI has unlocked Artillery!\n");
                    }
                    if (aiFleet->shipsDestroyed == 3 && !aiFleet->torpedoUnlocked)
                    {
                        aiFleet->torpedoUnlocked = 1;
                        printf("AI has unlocked Torpedo!\n");
                    }

                    return;
                }

                // Mark this target as attempted
                currentTarget->active = 0;

                return;
            }
            else
            {
                printf("AI misses at %c%d.\n", 'A' + y, x + 1);
                // Mark target as attempted
                currentTarget->active = 0;

                // Update turns since last hit
                aiTarget->turnsSinceLastHit++;

                return;
            }
        }
    }

    // If no active sinking targets, revert to searching
    printf("AI has no more sinking targets. Switching to Searching Mode.\n");
    aiTarget->state = STATE_SEARCHING;
}


// Helper functions

int isCellFeasible(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x,
                   int y)
{
    int shipSizes[5] = {fleet->carrier.size, fleet->battleship.size,
                        fleet->cruiser.size, fleet->submarine.size,
                        fleet->destroyer.size};
    int shipDestroyed[5] = {
        (fleet->carrier.hits == fleet->carrier.size),
        (fleet->battleship.hits == fleet->battleship.size),
        (fleet->cruiser.hits == fleet->cruiser.size),
        (fleet->submarine.hits == fleet->submarine.size),
        (fleet->destroyer.hits == fleet->destroyer.size)};

    for (int s = 0; s < 5; s++)
    {
        if (shipDestroyed[s])
            continue; // Skip destroyed ships
        int size = shipSizes[s];

        // Check horizontal feasibility
        if (y + size <= GRID_SIZE)
        {
            int canPlace = 1;
            for (int k = 0; k < size; k++)
            {
                if (grid[x][y + k] != '~')
                {
                    canPlace = 0;
                    break;
                }
            }
            if (canPlace)
                return 1;
        }

        // Check vertical feasibility
        if (x + size <= GRID_SIZE)
        {
            int canPlace = 1;
            for (int k = 0; k < size; k++)
            {
                if (grid[x + k][y] != '~')
                {
                    canPlace = 0;
                    break;
                }
            }
            if (canPlace)
                return 1;
        }
    }
    return 0; // No ship can be placed here
}

int areaAlreadyScanned(char grid[GRID_SIZE][GRID_SIZE], int x, int y)
{
    for (int i = x; i < x + 2 && i < GRID_SIZE; i++)
    {
        for (int j = y; j < y + 2 && j < GRID_SIZE; j++)
        {
            if (grid[i][j] == '~')
            {
                return 0; // Area not fully explored
            }
        }
    }
    return 1; // Area already explored
}

void getAdjacentTargets(char grid[GRID_SIZE][GRID_SIZE], int x, int y, AITargetMedium *aiTarget, int orientation)
{
    // Define possible directions based on orientation
    // 0 = Unknown, 1 = Horizontal, 2 = Vertical
    int directions[4][2];
    int numDirections = 0;

    if (orientation == 0)
    {
        // All four directions
        directions[0][0] = -1;
        directions[0][1] = 0; // Up
        directions[1][0] = 1;
        directions[1][1] = 0; // Down
        directions[2][0] = 0;
        directions[2][1] = -1; // Left
        directions[3][0] = 0;
        directions[3][1] = 1; // Right
        numDirections = 4;
    }
    else if (orientation == 1)
    {
        // Horizontal: Left and Right
        directions[0][0] = 0;
        directions[0][1] = -1; // Left
        directions[1][0] = 0;
        directions[1][1] = 1; // Right
        numDirections = 2;
    }
    else if (orientation == 2)
    {
        // Vertical: Up and Down
        directions[0][0] = -1;
        directions[0][1] = 0; // Up
        directions[1][0] = 1;
        directions[1][1] = 0; // Down
        numDirections = 2;
    }

    for (int i = 0; i < numDirections; i++)
    {
        int newX = x + directions[i][0];
        int newY = y + directions[i][1];

        // Check if the cell is within bounds and not already targeted
        if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE)
        {
            if (grid[newX][newY] != 'h' && grid[newX][newY] != 'm')
            {
                // Add to radar targets if not already present
                int exists = 0;
                for (int j = 0; j < aiTarget->radarTargetCount; j++)
                {
                    if (aiTarget->radarTargets[j].x == newX && aiTarget->radarTargets[j].y == newY)
                    {
                        exists = 1;
                        break;
                    }
                }
                if (!exists && aiTarget->radarTargetCount < 4)
                {
                    aiTarget->radarTargets[aiTarget->radarTargetCount].x = newX;
                    aiTarget->radarTargets[aiTarget->radarTargetCount].y = newY;
                    aiTarget->radarTargets[aiTarget->radarTargetCount].active = 1;
                    aiTarget->radarTargetCount++;
                    if (DEBUG_MODE)
                        printf("AI adds adjacent target at %c%d.\n", 'A' + newY, newX + 1);
                }
            }
        }
    }
}

ShipType isShipSunk(Fleet *fleet)
{
    if (fleet->carrier.hits == fleet->carrier.size && !fleet->carrierSunkReported)
    {
        fleet->carrierSunkReported = 1;
        return SHIP_CARRIER;
    }
    if (fleet->battleship.hits == fleet->battleship.size && !fleet->battleshipSunkReported)
    {
        fleet->battleshipSunkReported = 1;
        return SHIP_BATTLESHIP;
    }
    if (fleet->cruiser.hits == fleet->cruiser.size && !fleet->cruiserSunkReported)
    {
        fleet->cruiserSunkReported = 1;
        return SHIP_CRUISER;
    }
    if (fleet->submarine.hits == fleet->submarine.size && !fleet->submarineSunkReported)
    {
        fleet->submarineSunkReported = 1;
        return SHIP_SUBMARINE;
    }
    if (fleet->destroyer.hits == fleet->destroyer.size && !fleet->destroyerSunkReported)
    {
        fleet->destroyerSunkReported = 1;
        return SHIP_DESTROYER;
    }

    return SHIP_NONE; // No new ship has been sunk
}

char *getShipName(ShipType shipType)
{
    switch (shipType)
    {
    case SHIP_CARRIER:
        return "Carrier";
    case SHIP_BATTLESHIP:
        return "Battleship";
    case SHIP_CRUISER:
        return "Cruiser";
    case SHIP_SUBMARINE:
        return "Submarine";
    case SHIP_DESTROYER:
        return "Destroyer";
    default:
        return "Unknown";
    }
}

// Determines the orientation of the ship being sunk
void determineOrientation(AITargetMedium *aiTarget)
{
    if (aiTarget->hitCount >= 2 && aiTarget->orientation == 0)
    {
        int x0 = aiTarget->hits[0][0];
        int y0 = aiTarget->hits[0][1];
        int x1 = aiTarget->hits[1][0];
        int y1 = aiTarget->hits[1][1];

        if (x0 == x1)
        {
            aiTarget->orientation = 1; // Horizontal
        }
        else if (y0 == y1)
        {
            aiTarget->orientation = 2; // Vertical
        }
        else
        {
            aiTarget->orientation = 0; // Unknown
        }

        if (DEBUG_MODE)
            printf("AI has determined the ship's orientation: %s.\n",
                   aiTarget->orientation == 1 ? "Horizontal" : (aiTarget->orientation == 2 ? "Vertical" : "Unknown"));
    }
}

// Checks if the game is over
int isGameOver(Fleet *fleet)
{
    return fleet->shipsDestroyed == 5;
}

int main()
{
    srand(time(NULL));
    // Initialize grids
    char player1Grid[GRID_SIZE][GRID_SIZE];
    char player2Grid[GRID_SIZE][GRID_SIZE];
    char player1VisibleGrid[GRID_SIZE][GRID_SIZE];
    char player2VisibleGrid[GRID_SIZE][GRID_SIZE];

    // Initialize fleets and smoke screens
    Fleet player1Fleet, player2Fleet;
    SmokeScreen player1Smokes[MAX_SMOKE_SCREENS];
    SmokeScreen player2Smokes[MAX_SMOKE_SCREENS];
    int activeSmokeCount1 = 0, activeSmokeCount2 = 0;

    initializeFleet(&player1Fleet);
    initializeFleet(&player2Fleet);

    initGrid(player1Grid);
    initGrid(player2Grid);
    initGrid(player1VisibleGrid);
    initGrid(player2VisibleGrid);

    // Game mode selection
    int gameMode = 0; // 1 for Human vs Human, 2 for Human vs AI
    printf("Select Game Mode:\n");
    printf("1. Player vs Player\n");
    printf("2. Player vs AI\n");
    printf("Enter your choice: ");
    scanf("%d", &gameMode);
    while (gameMode != 1 && gameMode != 2)
    {
        printf("Invalid choice. Please select 1 for Player vs Player or 2 for Player vs AI: ");
        scanf("%d", &gameMode);
    }

    // Consume newline character left in the input buffer
    getchar();

    // AI difficulty selection if playing against AI
    if (gameMode == 2)
    {
        printf("Select AI Difficulty Level:\n");
        printf("1. Easy\n");
        printf("2. Medium\n");
        printf("3. Hard (Not yet implemented)\n");
        printf("Enter your choice: ");
        scanf("%d", &aiDifficulty);
        while (aiDifficulty < 1 || aiDifficulty > 3)
        {
            printf("Invalid choice. Please select 1 for Easy, 2 for Medium, or 3 for Hard: ");
            scanf("%d", &aiDifficulty);
        }
        getchar(); // Consume newline character
        if (aiDifficulty == 3)
        {
            printf("Hard AI is not yet implemented. Defaulting to Medium AI.\n");
            aiDifficulty = 2;
        }
    }

    // Place ships for Player 1 (predefined for simplicity)
    placeShip(player1Grid, &player1Fleet.carrier, 0, 0, 1);    // Carrier horizontally from A1
    placeShip(player1Grid, &player1Fleet.battleship, 2, 2, 0); // Battleship vertically from C3
    placeShip(player1Grid, &player1Fleet.cruiser, 5, 5, 1);    // Cruiser horizontally from F6
    placeShip(player1Grid, &player1Fleet.submarine, 7, 8, 0);  // Submarine vertically from I8
    placeShip(player1Grid, &player1Fleet.destroyer, 9, 1, 1);  // Destroyer horizontally from B10

    // Place ships for Player 2 or AI (predefined for simplicity)
    if (gameMode == 1)
    {
        // Player vs Player: Place ships for Player 2
        placeShip(player2Grid, &player2Fleet.carrier, 1, 1, 1);    // Carrier horizontally from B2
        placeShip(player2Grid, &player2Fleet.battleship, 3, 3, 0); // Battleship vertically from D4
        placeShip(player2Grid, &player2Fleet.cruiser, 6, 6, 1);    // Cruiser horizontally from G7
        placeShip(player2Grid, &player2Fleet.submarine, 8, 9, 0);  // Submarine vertically from J9
        placeShip(player2Grid, &player2Fleet.destroyer, 9, 0, 1);  // Destroyer horizontally from A10
    }
    else
    {
        // Player vs AI: Place ships for AI
        placeShip(player2Grid, &player2Fleet.carrier, 1, 1, 1);    // Carrier horizontally from B2
        placeShip(player2Grid, &player2Fleet.battleship, 3, 3, 0); // Battleship vertically from D4
        placeShip(player2Grid, &player2Fleet.cruiser, 6, 6, 1);    // Cruiser horizontally from G7
        placeShip(player2Grid, &player2Fleet.submarine, 8, 9, 0);  // Submarine vertically from J9
        placeShip(player2Grid, &player2Fleet.destroyer, 9, 0, 1);  // Destroyer horizontally from A10
    }

    // Initialize AI targets and smoke screens
    AITargetMedium aiTargetMedium;
    SmokeScreen aiSmokes[MAX_SMOKE_SCREENS];
    int activeSmokeCountAI = 0;
    int playerRadarUses = 0;

    if (gameMode == 2)
    {
        if (aiDifficulty == 2)
        {
            initAITargetMedium(&aiTargetMedium);
        }
    }

    srand(time(NULL));
    int turn = 1; // Player always starts first

    // Main game loop
    while (!isGameOver(&player1Fleet) && !isGameOver(&player2Fleet))
    {
        char input[32] = {0};
        char command[16] = {0};
        char target[5] = {0};
        int validCommand = 0;

        // Determine current and opponent fleets/grids based on turn
        Fleet *currentFleet = (turn == 1) ? &player1Fleet : &player2Fleet;
        Fleet *opponentFleet = (turn == 1) ? &player2Fleet : &player1Fleet;
        char(*currentGrid)[GRID_SIZE] = (turn == 1) ? player1Grid : player2Grid;
        char(*opponentGrid)[GRID_SIZE] = (turn == 1) ? player2Grid : player1Grid;
        char(*opponentVisibleGrid)[GRID_SIZE] = (turn == 1) ? player1VisibleGrid : player2VisibleGrid;
        SmokeScreen *currentSmokes = (turn == 1) ? player1Smokes : player2Smokes;
        int *activeSmokeCount = (turn == 1) ? &activeSmokeCount1 : &activeSmokeCount2;
        int *radarUses = &currentFleet->radarUses;

        // AI's turn
        if (gameMode == 2 && turn == 2)
{
    aiTurnMedium(opponentGrid, opponentFleet, currentFleet, &aiTargetMedium, currentSmokes, activeSmokeCount, playerRadarUses);

    // Reset player's radar uses for the AI's decision-making
    playerRadarUses = 0;

    // Update smoke screens after AI acts
    UpdateSmokeScreens(currentSmokes, activeSmokeCount);

    turn = 1; // Switch back to player
    continue;
}


        // Player's turn
        printf("\n=== Player %d's Turn ===\n", turn);
        displayGrids(turn, player1Grid, player2Grid, player1VisibleGrid, player2VisibleGrid, gameMode);

        // Update smoke screens before player's turn
        UpdateSmokeScreens(currentSmokes, activeSmokeCount);

        // Player inputs command
        while (!validCommand)
        {
            printf("Player %d, enter your command (e.g., Fire A5, Artillery A5, Torpedo A, Radar A5, Smoke A5): ", turn);
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0'; // Remove newline character

            if (sscanf(input, "%15s %4s", command, target) < 2)
            {
                printf("Invalid input format! Try again.\n");
                continue;
            }

            // Convert command to lowercase for case-insensitive comparison
            for (int i = 0; command[i]; i++)
            {
                command[i] = tolower(command[i]);
            }

            // Process the command
            if (strcmp(command, "fire") == 0 || strcmp(command, "artillery") == 0)
            {
                // Handle Fire and Artillery commands
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]))
                {
                    char colInput = toupper(target[0]);
                    int rowInput;
                    if (strlen(target) == 2 && isdigit(target[1]))
                    {
                        rowInput = target[1] - '0';
                    }
                    else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2]))
                    {
                        rowInput = (target[1] - '0') * 10 + (target[2] - '0');
                    }
                    else
                    {
                        printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                        continue;
                    }

                    if (colInput >= 'A' && colInput <= 'J' && rowInput >= 1 && rowInput <= 10)
                    {
                        validCommand = 1;
                        if (strcmp(command, "fire") == 0)
                        {
                            int result = Fire(opponentGrid, opponentFleet, rowInput - 1, colInput - 'A', NULL, 1);
                            // Update opponentVisibleGrid based on the result
                            if (result == 1 || result == 0) // Hit or Miss
                            {
                                opponentVisibleGrid[rowInput - 1][colInput - 'A'] = opponentGrid[rowInput - 1][colInput - 'A'];
                            }
                        }
                        else if (strcmp(command, "artillery") == 0)
                        {
                            if (currentFleet->artilleryUnlocked)
                            {
                                // Execute Artillery strike
                                Artillery(opponentGrid, opponentFleet, rowInput - 1, colInput - 'A', NULL, 1);

                                // Update opponentVisibleGrid for artillery area
                                for (int i = rowInput - 1; i < rowInput + 1 && i < GRID_SIZE; i++)
                                {
                                    for (int j = colInput - 'A'; j < colInput - 'A' + 2 && j < GRID_SIZE; j++)
                                    {
                                        if (opponentGrid[i][j] == 'h' || opponentGrid[i][j] == 'm')
                                        {
                                            opponentVisibleGrid[i][j] = opponentGrid[i][j];
                                        }
                                    }
                                }
                            }
                            else
                            {
                                printf("Artillery is not unlocked yet!\n");
                            }
                        }
                    }
                    else
                    {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                }
                else
                {
                    printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                }
            }
            else if (strcmp(command, "torpedo") == 0)
            {
                // Handle Torpedo command
                if (currentFleet->torpedoUnlocked)
                {
                    if (isalpha(target[0]) && strlen(target) == 1)
                    {
                        // Torpedo on a column
                        char colInput = toupper(target[0]);
                        if (colInput >= 'A' && colInput <= 'J')
                        {
                            validCommand = 1;
                            Torpedo(opponentGrid, opponentFleet, colInput - 'A', 'C', NULL, 1); // 'C' for column

                            // Update opponentVisibleGrid for torpedo
                            for (int i = 0; i < GRID_SIZE; i++)
                            {
                                if (opponentGrid[i][colInput - 'A'] == 'h' || opponentGrid[i][colInput - 'A'] == 'm')
                                {
                                    opponentVisibleGrid[i][colInput - 'A'] = opponentGrid[i][colInput - 'A'];
                                }
                            }
                        }
                        else
                        {
                            printf("Invalid column! Use A-J for columns.\n");
                        }
                    }
                    else if (isdigit(target[0]))
                    {
                        // Torpedo on a row
                        int rowInput = atoi(target);
                        if (rowInput >= 1 && rowInput <= 10)
                        {
                            validCommand = 1;
                            Torpedo(opponentGrid, opponentFleet, rowInput - 1, 'R', NULL, 1); // 'R' for row

                            // Update opponentVisibleGrid for torpedo
                            for (int j = 0; j < GRID_SIZE; j++)
                            {
                                if (opponentGrid[rowInput - 1][j] == 'h' || opponentGrid[rowInput - 1][j] == 'm')
                                {
                                    opponentVisibleGrid[rowInput - 1][j] = opponentGrid[rowInput - 1][j];
                                }
                            }
                        }
                        else
                        {
                            printf("Invalid row! Use 1-10 for rows.\n");
                        }
                    }
                    else
                    {
                        printf("Invalid target for Torpedo! Use a column (A-J) or row (1-10).\n");
                    }
                }
                else
                {
                    printf("Torpedo is not unlocked yet!\n");
                }
            }
            else if (strcmp(command, "radar") == 0)
            {
                // Handle Radar command
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]))
                {
                    char colInput = toupper(target[0]);
                    int rowInput;
                    if (strlen(target) == 2 && isdigit(target[1]))
                    {
                        rowInput = target[1] - '0';
                    }
                    else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2]))
                    {
                        rowInput = (target[1] - '0') * 10 + (target[2] - '0');
                    }
                    else
                    {
                        printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                        continue;
                    }

                    if (colInput >= 'A' && colInput <= 'J' && rowInput >= 1 && rowInput <= 10)
                    {
                        if (*radarUses >= MAX_RADAR_USES)
                        {
                            printf("No more radar sweeps available.\n");
                        }
                        else
                        {
                            validCommand = 1;
                            int enemyDetected = PlayerRadarSweep(opponentGrid, rowInput - 1, colInput - 'A', radarUses, currentSmokes, *activeSmokeCount);
                            playerRadarUses++; // Increment player's radar use
                        }
                    }
                    else
                    {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                }
                else
                {
                    printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                }
            }
            else if (strcmp(command, "smoke") == 0)
            {
                // Handle Smoke command
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]))
                {
                    char colInput = toupper(target[0]);
                    int rowInput;
                    if (strlen(target) == 2 && isdigit(target[1]))
                    {
                        rowInput = target[1] - '0';
                    }
                    else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2]))
                    {
                        rowInput = (target[1] - '0') * 10 + (target[2] - '0');
                    }
                    else
                    {
                        printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                        continue;
                    }

                    if (colInput >= 'A' && colInput <= 'J' && rowInput >= 1 && rowInput <= 10)
                    {
                        validCommand = 1;
                        DeploySmokeScreen(currentSmokes, activeSmokeCount, rowInput - 1, colInput - 'A', currentFleet);
                    }
                    else
                    {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                }
                else
                {
                    printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                }
            }
            else
            {
                printf("Unknown command '%s'. Available commands: Fire, Artillery, Torpedo, Radar, Smoke.\n", command);
            }
        }

        // Switch turns
        turn = (turn == 1) ? 2 : 1;
    }

    // Determine and announce the winner
    if (isGameOver(&player1Fleet))
    {
        if (gameMode == 2)
        {
            printf("\nGame Over. The AI has won.\n");
        }
        else
        {
            printf("\nPlayer 2 wins!\n");
        }
    }
    else
    {
        if (gameMode == 2)
        {
            printf("\nCongratulations! You have won the game.\n");
        }
        else
        {
            printf("\nPlayer 1 wins!\n");
        }
    }

    return 0;
}
