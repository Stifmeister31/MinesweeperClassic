#include "BaseMinesweeper.h"

#include <cmath>


void BaseMinesweeper::loadBoard(int w, int h, int m) {
        width = w;
        height = h;
        totalMines = m;
        grid.clear();
        grid.resize(width * height);

        // Pure flat memory calculation
        for (auto &cell: grid) {
                int index = static_cast<int>(&cell - &grid[0]);
                int cx = index % width;
                int cy = index / width;

                for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                                if (dx == 0 && dy == 0) {
                                        continue;
                                }
                                int nx = cx + dx;
                                int ny = cy + dy;

                                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                                        cell.neighbors.push_back(&grid[nx + ny * width]);
                                }
                        }
                }
        }
}

bool BaseMinesweeper::revealCell(int index) {
        if (index < 0 || index >= static_cast<int>(grid.size())) {
                return false;
        }
        Cell &cell = getCell(index);
        if (cell.isRevealed || cell.isFlagged) {
                return false;
        }
        cell.isRevealed = true;

        if (cell.neighboringMines == 0 && !cell.isMine) {
                for (Cell *neighbor: cell.neighbors) {
                        revealCell(neighbor - &grid[0]);
                }
        }
        return true;
}

void BaseMinesweeper::countNeighboringMines() {
        for (auto &cell : grid) {
                if (cell.isMine) {
                        continue;
                }
                int mineCount = 0;
                for (Cell *neighbor : cell.neighbors) {
                        if (neighbor->isMine) {
                                ++mineCount;
                        }
                }
                cell.neighboringMines = mineCount;
        }
}

void BaseMinesweeper::chordCell(int index) {
        Cell &cell = getCell(index);
        if (!cell.isRevealed || cell.neighboringMines == 0) {
                return;
        }
        // If the user placed enough flags, safely clear all remaining hidden tiles
        if (countNeighboringFlags(index) == cell.neighboringMines) {
                for (Cell *neighbor: cell.neighbors) {
                        if (!neighbor->isFlagged && !neighbor->isRevealed) {
                                revealCell(neighbor - &grid[0]);
                        }
                }
        }
}

int BaseMinesweeper::countNeighboringFlags(int index) const {
        const auto &cell = getCell(index);
        int count = 0;
        for (Cell *neighbor: cell.neighbors) {
                if (neighbor->isFlagged) {
                        ++count;
                }
        }
        return count;
}

int BaseMinesweeper::countUnrevealedNeighbors(int index) const {
        const auto &cell = getCell(index);
        int count = 0;
        for (Cell *neighbor: cell.neighbors) {
                if (!neighbor->isRevealed) {
                        ++count;
                }
        }
        return count;
}


void BaseMinesweeper::incrementFlag() {
        totalFlags++;
}

void BaseMinesweeper::decrementFlag() {
        totalFlags--;
}
