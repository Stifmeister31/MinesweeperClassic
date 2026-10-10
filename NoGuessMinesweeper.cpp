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
        int safetyMaxAttempts = 1000; // Prevent an infinite loop if constraints are impossible,
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
                        // Restore the grid state back to its default setup
                        // This hides the revealed blocks back under the safe fog
                        resetBoardState();
                        return;
                }
        }

        // Fallback: If 1000 boards failed, fallback to the last generated board to prevent a crash (improbable)
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

// creates an actual reduced linear equation system
void NoGuessMinesweeper::buildEquation(LinearEquationSystem &system) const {
        // total active frontier variables across all open clues.
        int totalVariables = static_cast<int>(system.variableToCellMap.size());
        // early exit guard
        if (totalVariables == 0) {
                return;
        }
        // iterates over the cells and ignores the null assigned frontier values to eliminate redundancy
        for (const auto &clueCell: grid) {

                if (clueCell.isRevealed && clueCell.neighboringMines > 0) {
                        //initialize a row
                        SparseRow row;
                        // total number of 1's
                        int currentFlagsCount = 0;

                        for (Cell *neighbor: clueCell.neighbors) {
                                // count the already placed flags to reduce
                                if (neighbor->isFlagged) {
                                        ++currentFlagsCount;
                                } else if (neighbor->id != -1) {
                                        // evaluate the neighbor and push if is an active frontier member
                                        row.activeVarIDs.push_back(neighbor->id);
                                }
                        }
                        //check if the row is satisfied
                        if (!row.activeVarIDs.empty()) {
                                // remaining unknown mines needed around this clue (clue count minus flags)
                                row.targetValue = clueCell.neighboringMines - currentFlagsCount;
                                // append to the equation system
                                system.matrixA.push_back(row);
                        }
                }
        }
}

// bounds them all together
LinearEquationSystem NoGuessMinesweeper::buildLinearSystem() {
        LinearEquationSystem system;
        assignFrontierIDs(system);
        buildEquation(system);
        return system;
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

        for (const auto &row: system.matrixA) {
                int activeCount = static_cast<int>(row.activeVarIDs.size());
                if (activeCount == 0) {
                        continue;
                }
                // the following are instant reducible guaranteed golden answers
                // If an equation sums to 0, every variable inside it MUST equal 0 (Safe).
                if (row.targetValue == 0) {
                        for (int varId: row.activeVarIDs) {
                                // the variable isn't a mine
                                deductions.push_back({system.variableToCellMap[varId], false});
                        }
                        return deductions;
                }
                // math Principle: Max Target (b = n)
                // If the target equals the number of variables, they MUST all equal 1 (Mines).
                if (row.targetValue == activeCount) {
                        for (int varId: row.activeVarIDs) {
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
        const auto &matrix = system.matrixA;
        int numRows = static_cast<int>(matrix.size());

        for (int i = 0; i < numRows; ++i) {
                // reassigned for readability
                const auto &rowI = matrix[i];
                // reduced row
                int activeInI = static_cast<int>(rowI.activeVarIDs.size());
                if (activeInI == 0) continue;

                for (int j = 0; j < numRows; ++j) {
                        //skip overlapping values
                        if (i == j) continue;
                        // reassigned
                        const auto &rowJ = matrix[j];
                        int activeInJ = static_cast<int>(rowJ.activeVarIDs.size());

                        // I can only be a strict subset of J if I touches fewer cells than J
                        if (activeInI >= activeInJ) {
                                continue;
                        }

                        //If every single hidden cell touching Clue I also touches Clue J then isSubset stays true.
                        bool isSubset = true;
                        // prove if it is true, look at the other clue and see if they overlap
                        for (int varI: rowI.activeVarIDs) {
                                bool foundInJ = false;
                                for (int varJ: rowJ.activeVarIDs) {
                                        if (varI == varJ) {
                                                foundInJ = true;
                                                break;
                                        }
                                }
                                if (!foundInJ) {
                                        isSubset = false;
                                        break;
                                }
                        }
                        // If strict subset is proven, execute linear combination: Row J - Row I
                        if (isSubset) {
                                int reducedTarget = rowJ.targetValue - rowI.targetValue;
                                // number of unique cells that only touch J in this context
                                int reducedActiveCount = activeInJ - activeInI;

                                // Form the new theoretical equation by skipping variables that were in I
                                std::vector<int> reducedActiveVars;
                                //thx gemini, limit memory allocation
                                reducedActiveVars.reserve(reducedActiveCount);
                                for (int varJ: rowJ.activeVarIDs) {
                                        bool inI = false;
                                        for (int varI: rowI.activeVarIDs) {
                                                if (varJ == varI) {
                                                        inI = true;
                                                        break;
                                                }
                                        }
                                        if (!inI) {
                                                reducedActiveVars.push_back(varJ);
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
