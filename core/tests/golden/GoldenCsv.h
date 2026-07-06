#pragma once

// -----------------------------------------------------------------------------
// GoldenCsv — minimal CSV reader for cross-engine validation fixtures.
//
// Loads a golden CSV (header row of column names + numeric data rows) produced
// by TMS Dynamics 3.0 and exposes column access by name. Test-only helper: it
// lives under tests/ and is never compiled into a shipping target, so it may use
// STL freely (the DLL-boundary rule in AGENTS.md does not apply here).
// -----------------------------------------------------------------------------

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace ymir::test
{

/** In-memory view of a golden CSV: header index + numeric rows. */
class GoldenCsv
{
public:
    explicit GoldenCsv(const std::string& path)
    {
        std::ifstream in(path);
        if (!in)
            throw std::runtime_error("GoldenCsv: cannot open " + path);

        auto stripCr = [](std::string& s) {
            if (!s.empty() && s.back() == '\r')
                s.pop_back();
        };

        std::string line;
        if (!std::getline(in, line))
            throw std::runtime_error("GoldenCsv: empty file " + path);
        stripCr(line);

        {
            std::stringstream ss(line);
            std::string       cell;
            int               col = 0;
            while (std::getline(ss, cell, ','))
                colIndex_[cell] = col++;
        }

        while (std::getline(in, line))
        {
            stripCr(line);
            if (line.empty())
                continue;
            std::vector<double> row;
            std::stringstream   ss(line);
            std::string         cell;
            while (std::getline(ss, cell, ','))
                row.push_back(std::stod(cell));
            rows_.push_back(std::move(row));
        }
    }

    /** Number of data rows (header excluded). */
    std::size_t rowCount() const noexcept { return rows_.size(); }

    /** Value at (row, column-name). Throws if the column is unknown. */
    double at(std::size_t row, const std::string& col) const
    {
        auto it = colIndex_.find(col);
        if (it == colIndex_.end())
            throw std::runtime_error("GoldenCsv: unknown column '" + col + "'");
        return rows_.at(row).at(static_cast<std::size_t>(it->second));
    }

private:
    std::unordered_map<std::string, int> colIndex_;
    std::vector<std::vector<double>>     rows_;
};

} // namespace ymir::test
