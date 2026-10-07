//
// Created by asimy on 6/25/2026.
//

#include "NoGuessMinesweeper.h"

#include <cmath>
#include <iostream>
#include <random>

void NoGuessMinesweeper::generateBoard(int firstClickX, int firstClickY) {
        //randomization
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distX(0, width - 1);
        std::uniform_int_distribution<> distY(0, height - 1);

        bool legalBoardFound = false;
        int safetyMaxAttempts = 1000; // Prevent an infinite loop if constraints are impossible
        int attempts = 0;

        while (attempts < safetyMaxAttempts && !legalBoardFound) {
                attempts++;

                // clear board
                for (auto &cell: grid) {
                        cell.isMine = false;
                        cell.isRevealed = false;
                        cell.isFlagged = false;
                        cell.id = -1;
                }

                // random placement with safe opening
                int placedMines = 0;
                while (placedMines < totalMines) {
                        int rx = distX(gen);
                        int ry = distY(gen);

                        // Block out the 3x3 safety quadrant around the starting click
                        if (std::abs(rx - firstClickX) <= 1 && std::abs(ry - firstClickY) <= 1) {
                                continue;
                        }

                        int targetIdx = rx + (ry * width);
                        if (grid[targetIdx].isMine) {
                                continue;
                        }

                        grid[targetIdx].isMine = true;
                        placedMines++;
                }

                //Precomputing coefficients
                countNeighboringMines();

                // backup
                std::vector<Cell> pristineLayoutBackup = grid;

                // Execute the first click virtually to open up the starting viewport cascade
                int firstClickIdx = firstClickX + firstClickY * width;
                revealCell(firstClickIdx);

                bool solverStalled = false;
                // if this is reached, mark as safe
                int totalSafeTiles = static_cast<int>(grid.size()) - totalMines;

                while (!solverStalled) {
                        // Count how many non-mine cells have been unsealed virtually
                        int currentRevealed = 0;
                        for (const auto &cell: grid) {
                                if (cell.isRevealed && !cell.isMine) {
                                        currentRevealed++;
                                }
                        }

                        // If virtual count matches total safe tiles, this configuration passes
                        if (currentRevealed == totalSafeTiles) {
                                legalBoardFound = true;
                                break;
                        }
                        LinearEquationSystem currentSystem = buildLinearSystem();
                        std::vector<Deduction> choices = executeAlgebraicPass(currentSystem);
                        if (choices.empty()) {
                                // The linear algebra engine cannot extract any more matrix transformations
                                solverStalled = true;
                        } else {
                                // Apply the extracted certainties straight into our virtual playthrough stream
                                for (const auto &move: choices) {
                                        // Grab the raw flat index of the referenced cell pointer
                                        int cellIdx = static_cast<int>(move.cellPtr - &grid[0]);

                                        if (move.isMine) {
                                                grid[cellIdx].isFlagged = true;
                                        } else {
                                                revealCell(cellIdx);
                                        }
                                }
                        }
                }
                if (legalBoardFound) {
                        // Restore the grid state back to its pristine post-generation setup
                        // This hides the revealed blocks back under the safe fog
                        grid = pristineLayoutBackup;
                        return;
                }
        }

        // Fallback: If 1000 boards failed, fallback to the last generated board to prevent a crash
        // TODO: print a mesage to let the user know
        for (auto &cell: grid) {
                cell.isRevealed = false;
                cell.isFlagged = false;
        }
        //count the last boards neighboring mines
        countNeighboringMines();
}

// Assign linear equation IDs x_n acting as pointers,
void NoGuessMinesweeper::assignFrontierIDs(LinearEquationSystem &system) {
        // assign id while resetting them first
        for (auto &cell: grid) {
                cell.id = -1;
                if (!cell.isRevealed && !cell.isFlagged) {
                        bool touchesClue = false;
                        //check if this cell touches an active clue (non-blank opened cells)
                        for (Cell *neighbor: cell.neighbors) {
                                if (neighbor->isRevealed && neighbor->neighboringMines > 0) {
                                        touchesClue = true;
                                        break;
                                }
                        }
                        // assign seq. IDs. i.e a contiguous front values are assigned and updated with each iteration
                        if (touchesClue) {
                                cell.id = static_cast<int>(system.variableToCellMap.size());
                                system.variableToCellMap.push_back(&cell);
                        }
                }
        }
}

// creates an actual linear equation system
void NoGuessMinesweeper::buildEquation(LinearEquationSystem &system) const {
        // builds the x_1 + x_2 + ... + x_n = k equation
        int totalVariables = static_cast<int>(system.variableToCellMap.size());
        // no valid front, collapse (edge case)
        if (totalVariables == 0) {
                return;
        }
        // Needs improvement but basically iterates over cells
        for (auto &clueCell: grid) {
                if (clueCell.isRevealed && clueCell.neighboringMines > 0) {
                        //initialize a row
                        MatrixRow row;
                        //start a clean row with totalVariables times 0
                        row.coefficients.assign(totalVariables, 0);

                        // total number of 1's
                        int activeVariablesInRow = 0;
                        int currentFlagsCount = 0;

                        // if the id is assigned (non-blank), then take it as a clue
                        for (Cell *neighbor: clueCell.neighbors) {
                                if (neighbor->isFlagged) {
                                        ++currentFlagsCount;
                                } else if (neighbor->id != -1) {
                                        row.coefficients[neighbor->id] = 1;
                                        ++activeVariablesInRow;
                                }
                        }
                        // set up the final equation and add the row to the solution matrix
                        if (activeVariablesInRow > 0) {
                                row.targetValue = clueCell.neighboringMines - currentFlagsCount;
                                system.matrixA.push_back(row);
                        }
                }
        }
}


LinearEquationSystem NoGuessMinesweeper::buildLinearSystem() {
        LinearEquationSystem system;
        assignFrontierIDs(system);
        buildEquation(system);
        return system;
}

//simplify the matrix eliminating the trivial variables
std::vector<int> NoGuessMinesweeper::getActiveVariables(const MatrixRow &row, int totalVariables) {
        std::vector<int> activeVars;
        for (int i = 0; i < totalVariables; ++i) {
                if (row.coefficients[i] == 1) {
                        activeVars.push_back(i);
                }
        }
        return activeVars;
}

//handle easy cases
std::vector<Deduction> NoGuessMinesweeper::executeAlgebraicPass(const LinearEquationSystem &system) {
        if (system.variableToCellMap.empty()) {
                return {};
        }
        //Try to find instant certainties without cross-referencing rows
        std::vector<Deduction> deductions = findTrivialDeductions(system);
        if (!deductions.empty()) {
                return deductions;
        }
        //If trivial logic fails, use row reduction to force a certainty
        return reduceOverlappingEquations(system);
}

//
std::vector<Deduction> NoGuessMinesweeper::findTrivialDeductions(const LinearEquationSystem &system) {
        //Placeholder that enables the algorithm to act upon certain cells
        std::vector<Deduction> deductions;
        //same, will work on it later. DRY
        int totalVars = static_cast<int>(system.variableToCellMap.size());
        // linear equation system
        for (const auto &row: system.matrixA) {
                std::vector<int> activeVars = getActiveVariables(row, totalVars);
                int activeCount = static_cast<int>(activeVars.size());
                //eliminate trivials
                if (activeCount == 0) {
                        continue;
                }
                // the following are instant reducible guaranteed golden answers
                // If an equation sums to 0, every variable inside it MUST equal 0 (Safe).
                if (row.targetValue == 0) {
                        for (int varId: activeVars) {
                                // the variable isn't a mine
                                deductions.push_back({system.variableToCellMap[varId], false});
                        }
                        return deductions;
                }
                // Math Principle: Max Target (b = n)
                // If the target equals the number of variables, they MUST all equal 1 (Mines).
                if (row.targetValue == activeCount) {
                        for (int varId: activeVars) {
                                deductions.push_back({system.variableToCellMap[varId], true});
                        }
                        return deductions;
                }
        }
        //return the finalized system
        return deductions;
}


//Matrix reduction
std::vector<Deduction> NoGuessMinesweeper::reduceOverlappingEquations(const LinearEquationSystem &system) {
        //same stuff
        std::vector<Deduction> deductions;
        int totalVars = static_cast<int>(system.variableToCellMap.size());
        const auto &matrix = system.matrixA;
        int numRows = static_cast<int>(matrix.size());

        //a matrix of lists that holds the variable id's for the specific row.
        // e.g rowActiveVars[0] = {1, 2} means that this row holds x_1 and x_2
        std::vector<std::vector<int>> rowActiveVars(numRows);


        //set up the matrix
        for (int i = 0; i < numRows; ++i) {
                rowActiveVars[i] = getActiveVariables(matrix[i], totalVars);
        }

        // nested loop
        for (int i = 0; i < numRows; ++i) {
                // how many clues touch i
                int activeInI = static_cast<int>(rowActiveVars[i].size());
                if (activeInI == 0) {
                        continue;
                }
                for (int j = 0; j < numRows; ++j) {
                        if (i == j) {
                                continue;
                        }
                        // how many clues touch j
                        int activeInJ = static_cast<int>(rowActiveVars[j].size());
                        // I can only be a subset of J if I has strictly hidden fewer variables than J.
                        // really important as it enables the algorithm to actually think instead of going off from obvious solution
                        if (activeInI >= activeInJ) {
                                continue;
                        }

                        //If every single hidden cell touching Clue I also touches Clue J then isSubset stays true.
                        bool isSubset = true;
                        // prove if it is true, look at the other clue and see if they overlap
                        for (int varId: rowActiveVars[i]) {
                                // if the clue does not touch this cell
                                if (matrix[j].coefficients[varId] != 1) {
                                        //the moment a single cell is found that clue i cares about but j doesn't,
                                        // the nested relationship is broken
                                        isSubset = false;
                                        break;
                                }
                        }
                        // If strict subset is proven, execute linear combination: Row J - Row I
                        if (isSubset) {
                                int reducedTarget = matrix[j].targetValue - matrix[i].targetValue;
                                // number of unique cells that only touch J in this context
                                int reducedActiveCount = activeInJ - activeInI;

                                // Form the new theoretical equation by skipping variables that were in I
                                std::vector<int> reducedActiveVars;
                                //thx gemini, limit memory allocation
                                reducedActiveVars.reserve(reducedActiveCount);

                                // save the non overlapping J variables
                                for (int varId: rowActiveVars[j]) {
                                        if (matrix[i].coefficients[varId] == 0) {
                                                reducedActiveVars.push_back(varId);
                                        }
                                }

                                // if the target drops to 0, then the unique cells belonging to J are safe and ready to open
                                if (reducedTarget == 0) {
                                        for (int varId: reducedActiveVars) {
                                                deductions.push_back({system.variableToCellMap[varId], false});
                                        }
                                        return deductions;
                                }
                                // every single unique cell contains a mine
                                if (reducedTarget == reducedActiveCount) {
                                        for (int varId: reducedActiveVars) {
                                                deductions.push_back({system.variableToCellMap[varId], true});
                                        }
                                        return deductions;
                                }
                        }
                }
        }
        // return the final mapping
        return deductions;
}
