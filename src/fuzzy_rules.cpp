#include "fuzzy_rules.h"

#include "tools.h"
#include <unicode/regex.h>
#include <memory>
#include <sstream>

std::unique_ptr<icu::RegexPattern> compileRegex(const std::string& regex) {
  UErrorCode status = U_ZERO_ERROR;
  auto matcher = std::unique_ptr<icu::RegexPattern>(icu::RegexPattern::compile(icu::UnicodeString(regex.c_str()), 0, status));
  if (U_FAILURE(status)) {
    throw std::runtime_error("Cannot parse Regex");
  }
  return matcher;
}

zim::FuzzyRule::FuzzyRule(const std::string& match)
: matchPatternString(match),
  matchPattern(compileRegex(match)),
  replaceString(""),
  replace(""),
  split_str("?"),
  splitlast(false)
{}

zim::FuzzyRule::FuzzyRule(const std::string& match, const std::string& replace, const std::string& split_str, bool splitlast, const std::vector<std::vector<std::string>>& args_list)
: matchPatternString(match),
  matchPattern(compileRegex(match)),
  replaceString(replace),
  replace(replace.c_str()),
  split_str(split_str),
  splitlast(splitlast),
  args_list(args_list)
{}

zim::FuzzyRule::FuzzyRule(const FuzzyRule& other)
: matchPatternString(other.matchPatternString),
  matchPattern(compileRegex(other.matchPatternString)),
  replaceString(other.replaceString),
  replace(other.replace),
  split_str(other.split_str),
  splitlast(other.splitlast),
  args_list(other.args_list)
{}

void zim::FuzzyRule::write(std::ostream& out) const {
  out << "MATCH " << matchPatternString << std::endl;
  out << "REPLACE " << replaceString << std::endl;
  if (!split_str.empty()) {
    if (splitlast) {
      out << "RSPLIT ";
    } else {
      out << "SPLIT ";
    }
    out << split_str << std::endl;
  }
  for(auto args: args_list) {
    std::string sep = "ARGS ";
    for(auto& arg: args) {
      out << sep << arg;
      sep = "&";
    }
    out << std::endl;
  }
}

zim::FuzzyRules::FuzzyRules(const std::string& fuzzyRulesData) {
  std::istringstream ss(fuzzyRulesData);

  std::unique_ptr<zim::FuzzyRule> currentRule;

  while (ss) {
    std::string line;
    getline(ss, line);
    const std::string::size_type k = line.find(" ");
    if ( k != std::string::npos) {
      const std::string order = line.substr(0, k);
      const std::string value = line.substr(k+1);
      if (order == "MATCH") {
        if (currentRule) {
          rules.emplace_back(std::move(*currentRule.release()));
        }
        currentRule = std::make_unique<FuzzyRule>(value);
      } else if (order == "REPLACE") {
        if (!currentRule) {
          continue;
        }
        currentRule->setReplace(value);
      } else if (order == "SPLIT") {
        if (!currentRule) {
          continue;
        }
        currentRule->split(value, false);
      }
      else if (order == "RSPLIT") {
        if (!currentRule) {
          continue;
        }
        currentRule->split(value, true);
      } else if (order == "ARGS") {
        if (!currentRule) {
          continue;
        }
        currentRule->add_try_args(split(value, "&"));
      } else {
        // unknow order
      }
    }
  }

  if (currentRule) {
    rules.emplace_back(std::move(*currentRule.release()));
  }
}

const zim::FuzzyRule& zim::FuzzyRules::get_rule(icu::UnicodeString& uPath) const {
 for ( const auto& fuzzy_rule : rules ) {
    UErrorCode status = U_ZERO_ERROR;
    auto matcher = std::unique_ptr<icu::RegexMatcher>(fuzzy_rule.matchPattern->matcher(uPath, status));
    if (U_FAILURE(status)) {
      // skip this rules if we have any error
      continue;
    }
    if (matcher->find()) {
      return fuzzy_rule;
    }
  }
  throw std::runtime_error("No Rule");
}

std::string get_optional_query_param(const std::vector<std::pair<std::string, std::string>>& queryParams, const std::string& name) {
    for(const auto& queryParam: queryParams) {
      if (queryParam.first == name) {
        return queryParam.second;
      }
    }
    return "";
}

std::vector<std::string> zim::FuzzyRules::get_fuzzy_paths(const std::string& path, const std::vector<std::pair<std::string, std::string>>& queryParams) const {
  std::vector<std::string> fuzzy_urls;

  // First of all, add the query_string
  auto url_queried = path + "?";
  auto sep = "";
  for(const auto& queryParam: queryParams) {
    url_queried += sep + queryParam.first + "=" + queryParam.second;
    sep = "&";
  }
  fuzzy_urls.push_back(url_queried);


  try {
    auto u_url_queried = icu::UnicodeString(url_queried.c_str());
    auto& rule = get_rule(u_url_queried);

    std::string fuzzy_cannon_url;
    if (!rule.replace.isEmpty()) {
      UErrorCode status = U_ZERO_ERROR;
      auto matcher = std::unique_ptr<icu::RegexMatcher>(rule.matchPattern->matcher(u_url_queried, status));
      auto u_fuzzy_cannon_url = matcher->replaceAll(rule.replace, status);
      // Ignore status. What to do in case of error ? Ignore rules, continue, fail ?
      u_fuzzy_cannon_url.toUTF8String(fuzzy_cannon_url);
    } else {
      auto split_idx = rule.splitlast ? url_queried.rfind(rule.split_str) : url_queried.find(rule.split_str);
      if (split_idx == std::string::npos) {
        fuzzy_cannon_url = url_queried;
      } else {
        fuzzy_cannon_url = url_queried.substr(0, split_idx+rule.split_str.size());
      }
    }

    // remove any left querystring from fuzzy_cannon_url.
    auto split_idx = fuzzy_cannon_url.find("?");
    fuzzy_cannon_url = fuzzy_cannon_url.substr(0, split_idx);

    fuzzy_urls.push_back(fuzzy_cannon_url);

    for (auto args: rule.args_list) {
      std::stringstream query;
      std::string sep="?";
      for (auto arg: args) {
        query << sep << arg << "=" << get_optional_query_param(queryParams, arg);
        sep = "&";
      }
      fuzzy_urls.push_back(fuzzy_cannon_url+query.str());
    }
  } catch(const std::runtime_error&) {
    auto split_idx = url_queried.find("?");
    fuzzy_urls.push_back(split_idx == std::string::npos ? url_queried : url_queried.substr(0, split_idx+1));
  }
  return fuzzy_urls;
}
