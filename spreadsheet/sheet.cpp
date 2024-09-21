#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if(pos.IsValid()) {
        const auto& cell = cells_.find(pos);

        if(cell == cells_.end()) {
            cells_.emplace(pos, std::make_unique<Cell>(*this));
        }
        cells_.at(pos)->Set(std::move(text));
    } else {
        throw InvalidPositionException("Pos is invalid!");
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if(pos.IsValid()) {
        const auto cell = cells_.find(pos);
        if(cell == cells_.end()) {
            return nullptr;
        } else {
            return cell->second.get();
        }
    } else {
        throw InvalidPositionException("Pos is invalid!");
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if(pos.IsValid()) {
        const auto& cell = cells_.find(pos);
        if(cell == cells_.end()) {
            return nullptr;
        } else {
            return cell->second.get();
        }
    } else {
        throw InvalidPositionException("Pos is invalid!");
    }
}

void Sheet::ClearCell(Position pos) {
    if(pos.IsValid()) {
        const auto& cell = cells_.find(pos);
        if(cell != cells_.end() && cell->second != nullptr) {
            cell->second->Clear();
            if(!cell->second->IsReferenced()) {
                cell->second.reset();
            }
        }
    } else {
        throw InvalidPositionException("Pos is invalid!");
    }
}

Size Sheet::GetPrintableSize() const {
    Size result = Size{0, 0};

    for(auto cell = cells_.begin(); cell != cells_.end(); ++cell) {
        if(cell->second != nullptr) {
            const int col = cell->first.col;
            const int row = cell->first.row;
            result.cols = std::max(result.cols, col + 1);
            result.rows = std::max(result.rows, row + 1);
        }
    }

    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();

    for(int row = 0; row < size.rows; ++row) {
        for(int col = 0; col < size.cols; ++col) {
            if(col > 0) {
                output << "\t";
            }
            const auto& cell = cells_.find({row, col});

            if(cell != cells_.end() && cell->second != nullptr && !cell->second->GetText().empty()) {
                std::visit([&](const auto value) { output << value; }, cell->second->GetValue());
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();

    for(int row = 0; row < size.rows; ++row) {
        for(int col = 0; col < size.cols; ++col) {
            if(col > 0) {
                output << "\t";
            }
            
            const auto& cell = cells_.find({row, col});

            if(cell != cells_.end() && cell->second != nullptr && !cell->second->GetText().empty()) {
                output << cell->second->GetText();
            }
        }
        output << "\n";
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if(pos.IsValid()) {
        return const_cast<Cell*>(GetConcreteCell(pos));
    } else {
        throw InvalidPositionException("Pos is invalid!");
    }
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if(pos.IsValid()) {
        const auto cell = cells_.find(pos);
        if(cell == cells_.end()) {
            return nullptr;
        } else {
            return cell->second.get();
        }
    } else {
        throw InvalidPositionException("Pos is invalid!");
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}