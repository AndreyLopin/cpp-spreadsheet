#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    
    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    }
    virtual bool IsValidCache() const {
        return true;
    }
    virtual void InvalidateCache() {
    }
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return "";
    }
    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string text)
        : text_(std::move(text)) {
        if(text_.empty()) {
            throw std::logic_error("Empty text in cell.");
        }
    }
    Value GetValue() const override {
        if(text_.at(0) == ESCAPE_SIGN) {
            return text_.substr(1);
        } else {
            return text_;
        }
    }
    std::string GetText() const override {
        return text_;
    }
private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(std::string text, const SheetInterface& sheet)
        : sheet_(sheet) {
        if(text.empty() || text.at(0) != FORMULA_SIGN) {
            throw std::logic_error("This isn't formula or empty text in cell.");
        }
        formula_ptr_ = ParseFormula(text.substr(1));
    }

    Value GetValue() const override {
        if(!cache_) {
            cache_ = formula_ptr_->Evaluate(sheet_);
        }

        auto result = formula_ptr_->Evaluate(sheet_);
        if(std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        } else {
            return std::get<FormulaError>(result);
        }
    }

    std::string GetText() const override {
        return (FORMULA_SIGN + formula_ptr_->GetExpression());
    }

    bool IsValidCache() const override {
        return cache_.has_value();
    }

    void InvalidateCache() override  {
        cache_.reset();
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_ptr_->GetReferencedCells();
    }
private:
    std::unique_ptr<FormulaInterface> formula_ptr_;
    const SheetInterface& sheet_;
    mutable std::optional<FormulaInterface::Value> cache_;
};

bool Cell::IsCircularDependency(const Cell::Impl& impl) const {
    if (impl.GetReferencedCells().empty()) {
        return false;
    }

    std::unordered_set<const Cell*> referenced;
    for (const auto& pos : impl.GetReferencedCells()) {
        referenced.insert(sheet_.GetConcreteCell(pos));
    }

    std::unordered_set<const Cell*> visited;
    std::stack<const Cell*> to_visit;
    to_visit.push(this);
    while (!to_visit.empty()) {
        const Cell* current = to_visit.top();
        to_visit.pop();
        visited.insert(current);

        if (referenced.find(current) != referenced.end()) {
            return true;
        }

        for (const Cell* incoming : current->depended_cells_) {
            if (visited.find(incoming) == visited.end()) {
                to_visit.push(incoming);
            }
        }
    }

    return false;
}

void Cell::InvalidateCacheDependedCells() {
    if(impl_->IsValidCache()) {
        impl_->InvalidateCache();

        for(Cell* cell : depended_cells_) {
            cell->InvalidateCacheDependedCells();
        }
    }
}

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<Cell::EmptyImpl>())
    , sheet_(sheet) {
}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;
    if(text.empty()) {
        impl = std::make_unique<Cell::EmptyImpl>();
    } else if(text.size() > 1 && text[0] == FORMULA_SIGN) {
        impl = std::make_unique<Cell::FormulaImpl>(std::move(text), sheet_);
    } else {
        impl = std::make_unique<Cell::TextImpl>(std::move(text));
    }

    if(IsCircularDependency(*impl)) {
        throw CircularDependencyException("Cell has circular dependency");
    }

    impl_ = std::move(impl);

    for (Cell* outgoing : referenced_cells_) {
        outgoing->depended_cells_.erase(this);
    }

    referenced_cells_.clear();

    for (const auto& pos : impl_->GetReferencedCells()) {
        Cell* outgoing = sheet_.GetConcreteCell(pos);
        if (!outgoing) {
            sheet_.SetCell(pos, "");
            outgoing = sheet_.GetConcreteCell(pos);
        }
        referenced_cells_.insert(outgoing);
        outgoing->depended_cells_.insert(this);
    }

    InvalidateCacheDependedCells();
}

void Cell::Clear() {
    impl_ = std::make_unique<Cell::EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !depended_cells_.empty();
}