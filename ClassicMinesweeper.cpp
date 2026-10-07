#include "ClassicMinesweeper.h"

#include <cmath>
#include <random>

#include "iostream"


void ClassicMinesweeper::generateBoard(int firstClickX, int firstClickY) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distX(0, width - 1);
        std::uniform_int_distribution<> distY(0, height - 1);

        int placedMines = 0;
        while (placedMines < totalMines) {
                int rx = distX(gen);
                int ry = distY(gen);

                bool isInsideSafetyZone = false;

                if (useSafeOpening) {
                        // Modern Mode: Block the entire 3x3 grid around the first click
                        isInsideSafetyZone = std::abs(rx - firstClickX) <= 1 && std::abs(ry - firstClickY) <= 1;
                } else {
                        // TRUE Old School Experience: Only block the exact pixel clicked
                        isInsideSafetyZone = rx == firstClickX && ry == firstClickY;
                }
                // 2. Reject if it falls in the safety zone OR if a mine is already there
                int targetIndex = getCellID(rx, ry);
                if (isInsideSafetyZone || getCell(targetIndex).isMine) {
                        continue;
                }
                getCell(targetIndex).isMine = true;
                placedMines++;
        }
        countNeighboringMines();
}


