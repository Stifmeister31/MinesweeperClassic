//
// Created by asimy on 6/25/2026.
//

#ifndef MINESWEEPERCLASSIC_BASEMINESWEEPER_H

#define MINESWEEPERCLASSIC_BASEMINESWEEPER_H

#include <vector>

struct Cell {
        bool isMine = false;
        bool isRevealed = false;
        bool isFlagged = false;
        int neighboringFlags = 0;
        int neighboringMines = 0;
        int unOpenedNeighbors = 0;

        std::vector<Cell*> neighbors;

        int id = -1;
};

class BaseMinesweeper {
protected:
        int width = 0;
        int height = 0;
        int totalMines = 0;
        int totalFlags = 0;
        std::vector<Cell> grid;
        bool useSafeOpening = true;
        [[nodiscard]] bool isValid(int x, int y) const {
                return x >= 0 && x < width && y >= 0 && y < height;
        }
        void countFlags(int x, int y);

public:
        [[nodiscard]] int total_flags() const {
                return totalFlags;
        }

        virtual ~BaseMinesweeper() = default;

        //return mutable, modifiable objects
        [[nodiscard]] Cell &getCell(int index) {
                return grid[index];
        }

        // return immutable, unmodifiable objects, read-only
        [[nodiscard]] const Cell &getCell(int index) const {
                return grid[index];
        }

        [[nodiscard]] int getCellID(int x, int y) {
                return x + (y * width);
        }

        [[nodiscard]] int total_mines() const {
                return totalMines;
        }

        void incrementFlag();
        void decrementFlag();

        bool revealCell(int index);

        void countNeighboringMines();

        void loadBoard(int w, int h, int m);
        void chordCell(int index);
        [[nodiscard]] const std::vector<Cell>& getGrid() const {
                return grid;
        }

        virtual void generateBoard(int firstClickX, int firstClickY) = 0;

        [[nodiscard]] int countNeighboringFlags(int index) const;
        [[nodiscard]] int countNeighboringMines(int x, int y) const;
        [[nodiscard]] int countUnrevealedNeighbors(int index) const;


        [[nodiscard]] int getWidth() const {
                return width;
        }

        [[nodiscard]] int getHeight() const {
                return height;
        }

};
#endif