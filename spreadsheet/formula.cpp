#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    output << fe.ToString();
    return output;
};

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) 
        try : ast_(ParseFormulaAST(expression)) {}
        catch (const std::exception& e) {
            std::throw_with_nested(FormulaException(e.what()));
    }
    
    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            const std::function<double(Position)> args = [&sheet](const Position pos)->double {
                if(pos.IsValid()) {
                    const auto* cell = sheet.GetCell(pos);
                    if(!cell) {
                        return 0;
                    }
                    if(std::holds_alternative<double>(cell->GetValue())) {
                        return std::get<double>(cell->GetValue());
                    }
                    if(std::holds_alternative<std::string>(cell->GetValue())) {
                        auto str = std::get<std::string>(cell->GetValue());
                            
                        double result = 0;
                        if(!str.empty()) {
                            std::istringstream input(str);

                            if(!(input >> result) || !input.eof()) {
                                throw FormulaError(FormulaError::Category::Value);
                            }
                        }
                        return result;
                    }
                    throw FormulaError(std::get<FormulaError>(cell->GetValue()));
                } else {
                    throw FormulaError(FormulaError::Category::Ref);
                }
            };

            return ast_.Execute(args);
        } catch (const FormulaError& error) {
            return error;
        }
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        for(auto cell : ast_.GetCells()) {
            if(cell.IsValid()) {
                cells.emplace_back(std::move(cell));
            }
        }
        cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
        return cells;
    }

    std::string GetExpression() const override {
        std::ostringstream result;
        ast_.PrintFormula(result);
        return result.str();
    }

private:
    const FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Error create formula");
    }
}