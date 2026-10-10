//
// Created by asimy on 6/25/2026.
//

#ifndef MINESWEEPERCLASSIC_NOGUESSMINESWEEPER_H
#define MINESWEEPERCLASSIC_NOGUESSMINESWEEPER_H
#include "BaseMinesweeper.h"

// The final answer, holds if the cell is a mine or not
struct Deduction {
        Cell *cellPtr;
        bool isMine;
};

// Holds the boolean coefficients as well as the solution parameter
// struct MatrixRow {
//         std::vector<int> coefficients;
//         int targetValue = 0;
// };

// Holds the boolean coefficients as well as the dynamic solution parameter.
struct SparseRow {
        std::vector<int> activeVarIDs;
        int targetValue;
};

// A linear equation system that updates with each expansion
struct LinearEquationSystem {
        std::vector<SparseRow> matrixA;
        std::vector<Cell *> variableToCellMap;
};


class NoGuessMinesweeper : public BaseMinesweeper {
private:


        void assignFrontierIDs(LinearEquationSystem &);

        void buildEquation(LinearEquationSystem &) const;

        LinearEquationSystem buildLinearSystem();

        static std::vector<Deduction> findTrivialDeductions(const LinearEquationSystem &system);

        static std::vector<Deduction> reduceOverlappingEquations(const LinearEquationSystem &system);

        static std::vector<Deduction> executeAlgebraicPass(const LinearEquationSystem &system);

public:
        void generateBoard(int firstClickX, int firstClickY) override;
};


#endif //MINESWEEPERCLASSIC_NOGUESSMINESWEEPER_H
