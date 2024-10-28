#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GRID_SIZE 10

// keeps track of placed ships on grid1
// must be reset for player 2
char placedShips [6] = "NNNNN";

// Ship structure to define each ship type
typedef struct {
    char type;
    int size;
    int hits;  // Tracks hits for each ship
} Ship;

// Fleet structure to define a collection of ships for each player
typedef struct {
    Ship carrier;
    Ship battleship;
    Ship cruiser;
    Ship submarine;
    Ship destroyer;
} Fleet;

// Function prototypes
void initGrid(char grid[GRID_SIZE][GRID_SIZE]);
void printGrid(char grid[GRID_SIZE][GRID_SIZE]);
void initializeFleet(Fleet *fleet);
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation);
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]);

int main() {
    char grid1[GRID_SIZE][GRID_SIZE];
    char grid2[GRID_SIZE][GRID_SIZE];

    // These're displayed at opponent's turn
    char grid1for2[GRID_SIZE][GRID_SIZE];
    char grid2for1[GRID_SIZE][GRID_SIZE];
    
    Fleet fleet1, fleet2;
    initializeFleet(&fleet1);
    initializeFleet(&fleet2);

    initGrid(grid1);
    initGrid(grid2);
    printGrid(grid1);

    initGrid(grid1for2);
    initGrid(grid2for1);

    int x, y, orientation;
    char shipType;
    Ship *ship;
    
    printf("Enter ship type (C for Carrier, B for Battleship, c for Cruiser, s for Submarine, d for Destroyer), coordinates (row and column), and orientation (0 for horizontal, 1 for vertical):\n");

    for (int i = 0; i < 5; i++) {  // 5 ships per player
        printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
        scanf(" %c %d %d %d", &shipType, &x, &y, &orientation);
        x--;  // Convert to 0-based index
        y--;

        // Determine which ship to place
        switch (shipType) {
            case 'C': ship = &fleet1.carrier; break;
            case 'B': ship = &fleet1.battleship; break;
            case 'c': ship = &fleet1.cruiser; break;
            case 's': ship = &fleet1.submarine; break;
            case 'd': ship = &fleet1.destroyer; break;
            default: 
                printf("Invalid ship type. Try again.\n");
                i--;  // Repeat this step
                continue;
        }

        // Place the ship and check for valid placement
        if (placeShip(grid1, ship, x, y, orientation) == 0) {
            printf("Ship placed successfully.\n");
            printGrid(grid1);
        } else {
            printf("Invalid placement. Try again.\n");
            i--;  // Repeat this step
        }
    }

    // Now, it is time to start the game
    // Program decides randomly whom to go first
    srand(time(NULL));

    int turn = rand() % 2 + 1;

    displayGrids(turn, grid1, grid2, grid1for2, grid2for1);

    return 0;
}

// Initialize each fleet with its ships and sizes
void initializeFleet(Fleet *fleet) {
    fleet->carrier = (Ship){'C', 5, 0};
    fleet->battleship = (Ship){'B', 4, 0};
    fleet->cruiser = (Ship){'c', 3, 0};
    fleet->submarine = (Ship){'s', 3, 0};
    fleet->destroyer = (Ship){'d', 2, 0};
}

// Initialize the grid with '~' for water
void initGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '~';
        }
    }
}

// Print the grid with column and row headers
void printGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    printf("   A B C D E F G H I J\n");  // Column headers
    for (int i = 0; i < GRID_SIZE; i++) {
        printf("%2d ", i + 1);  // Row numbers
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
}


// returns 1 if ship's already placed on the grid
// or 0 if it hasn't yet been placed
// and then adds it to the placedShips array
int isPlaced(Ship *ship){
    int j = 0;
    for(int i = 0; i < 6; i++){
        if(placedShips[i] != 'N')
            j++;
        else
            break;
    }

    for(int i = 0; i < 6; i++){
        if(placedShips[i] == ship->type){
            printf("This ship has already been placed on the grid.\n");
            return 1;
        }
    }
    placedShips[j] = ship->type;
    return 0;
}


// Place a ship on the grid based on coordinates and orientation
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation) {
    // Check bounds and availability of cells
    if (orientation == 0) {  // Horizontal placement
        if (y + ship->size > GRID_SIZE) return -1;  // Out of bounds
        for (int i = 0; i < ship->size; i++) {
            if (grid[x][y + i] != '~') return -1;  // Spot occupied
        }
        if (isPlaced(ship) == 1) 
            return -1; // ship has already been placed
        for (int i = 0; i < ship->size; i++) {
            grid[x][y + i] = ship->type;
        }
    } else if (orientation == 1) {  // Vertical placement
        if (x + ship->size > GRID_SIZE) return -1;  // Out of bounds
        for (int i = 0; i < ship->size; i++) {
            if (grid[x + i][y] != '~') return -1;  // Spot occupied
        }
        if (isPlaced(ship) == 1) 
            return -1; // ship has already been placed
        for (int i = 0; i < ship->size; i++) {
            grid[x + i][y] = ship->type;
        }
    } else {
        return -1;  // Invalid orientation
    }
    return 0;  // Successful placement
}


// A player in the easy mode should
// be able to see his own grid
// and his opponent's grid
// with prev Ms and Hs
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]){
    // it's first Player's turn to hit
    if(turn == 1){
        printf("Here is your grid: \n")
        printGrid(grid1);
        printf("Here is Player 2's grid: \n")
        printGrid(grid2for1);
    }else{
        printf("Here is your grid: \n")
        printGrid(grid2);
        printf("Here is Player 1's grid: \n")
        printGrid(grid1for2);
    }
}