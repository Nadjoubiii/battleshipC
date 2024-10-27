#include <stdio.h>
#include <stdlib.h>

void printGrid();
void initGrid(char arr[10][10]);

typedef fleet{
    // this is to count the hits
    int carrier, battleship,cruiser, submarine, destroyer
   
}

int main()
{

    // For grids ~ is empty, h is hit, m is miss
    char grid1[10][10];
    char grid2[10][10]; 

    // each player has a fleet of 5 ships, each array entry representing a ship
    int fleet1[5]; 
    int fleet2[5];
    // For entries ~ = empty, h = hit, m = miss | e in hard, b = battleship, C = carrier, c = cruiser, s = submarine, d = destroyer


    initGrid(grid1);
    initGrid(grid2);


    int trackingDifficulty; // 0 for easy, 1 for hard
    printGrid();
    printf("Please Enter the tracking difficult: \n");
    // scanf("%d",&trackingDifficulty);
    // printf("%d",trackingDifficulty);

    
    int roll = rand() % (2 + 1);
    
    printf("Ship positioning: \n");
    printf("Enter ship letter followed by coordinates and orientation \nEx: B3,h (Battleship,3 horizontal): \n");


    return 0;
}

void placeShip(char grid[10][10], char ship, int pos, int orientation){
    if( orientation == 0){
        if (pos
    }
    if(orientation == 1){

    }
    else{
        printf("incorrect orientation, try again\n");
    }
}


void initGrid(char arr[10][10]){
    for(int i = 0; i<10; i++){
        for(int j = 0; j<10;j++){
            arr[i][j] = '~';
        }
    }
}


void printGrid() {
    char columns[] = "A B C D E F G H I J";
    printf("   %s\n", columns); 

    for (int i = 1; i <= 10; i++) {
        printf("%2d ", i);      
        for (int j = 0; j < 10; j++) {
            printf("~ ");       
        }
        printf("\n");
    }
}
