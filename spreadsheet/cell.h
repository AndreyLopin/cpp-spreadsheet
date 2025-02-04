#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    bool IsCircularDependency(const Impl& impl) const;
    void InvalidateCacheDependedCells();
    
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;

    // Добавьте поля и методы для связи с таблицей, проверки циклических 
    // зависимостей, графа зависимостей и т. д.
    std::unordered_set<Cell*> referenced_cells_; // ячейки, которые зависят от этой ячейки
    std::unordered_set<Cell*> depended_cells_; // ячейки, от которых зависит эта ячейка
};