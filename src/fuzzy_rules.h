/*
 * Copyright (C) 2016-2020 Matthieu Gautier <mgautier@kymeria.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef OPENZIM_LIBZIM_FUZZYRULES_H
#define OPENZIM_LIBZIM_FUZZYRULES_H

#include <string>
#include <vector>
#include <ostream>
#include <unicode/regex.h>

namespace zim {

class FuzzyRule {
  public:
    explicit FuzzyRule(const std::string& match);
    FuzzyRule(const std::string& match, const std::string& replace, const std::string& split_str, bool splitlast, const std::vector<std::vector<std::string>>& arg_list);
    FuzzyRule(FuzzyRule&&) = default;
    FuzzyRule(const FuzzyRule&);

    void setReplace(const std::string& replace) {
      this->replaceString = replace;
      this->replace = icu::UnicodeString(replace.c_str());
    }

    void split(const std::string& split, bool last) {
      split_str = split;
      splitlast = last;
    }

    void add_try_args(std::vector<std::string> args) {
      args_list.push_back(args);
    }

    void write(std::ostream& out) const;

  private:
   std::string matchPatternString;
   std::unique_ptr<icu::RegexPattern> matchPattern;
   std::string replaceString;
   icu::UnicodeString replace;
   std::string split_str;
   bool splitlast;
   std::vector<std::vector<std::string>> args_list;
   friend class FuzzyRules;

   friend bool operator==(const FuzzyRule& lhs, const FuzzyRule& rhs) {
     return std::make_tuple(lhs.matchPatternString, lhs.replaceString, lhs.split_str, lhs.splitlast, lhs.args_list)
      == std::make_tuple(rhs.matchPatternString, rhs.replaceString, rhs.split_str, rhs.splitlast, rhs.args_list);
   }

   friend void PrintTo(const FuzzyRule& rule, std::ostream* os) {
    *os << "(match:" << rule.matchPatternString << " replace:" << rule.replaceString << " split:" << rule.split_str << "," << rule.splitlast << " args:[";
    for (const auto& args: rule.args_list) {
      *os << "(";
      for(const auto& arg: args) {
        *os << arg << ",";
      }
      *os << "),";
    }
    *os << "])\n";
   }

};


class FuzzyRules {
  public:
    FuzzyRules() = default;
    explicit FuzzyRules(const std::string& fuzzyRulesData);

    std::vector<std::string> get_fuzzy_paths(const std::string& path, const std::vector<std::pair<std::string, std::string>>& queryParams) const;
std::vector<FuzzyRule> rules;
  private:


    const FuzzyRule& get_rule(icu::UnicodeString& path) const;
};

} // end of namespace zim


#endif // OPENZIM_LIBZIM_FUZZYRULES_H
