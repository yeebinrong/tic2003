#include "QueryProcessor.h"
#include "Tokenizer.h"
#include <iostream>
#include <map>
#include <string>
#include <algorithm>

// constructor
QueryProcessor::QueryProcessor() {}

// destructor
QueryProcessor::~QueryProcessor() {}

// method to append "and" if clause is not empty
string appendAnd(string clause) {
	if (clause != "") {
		return clause + " AND ";
	}
	return clause;
}

// method to check if value is found in the vector
bool isValInVectTwo(vector<string> vector, string value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

bool isValInMap(map<string, string> m, string value) {
	if (m.find(value) == m.end()) {
		return false;
	}
	return true;
}

bool isExactMatch(string str) {
	if (str.find('_') < str.length()) {
		return false;
	}
	return true;
}

string removeWhiteSpace(string str) {
	str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
	return str;
}

string checkAndReplaceLike(string str) {
	if (str[0] == '_') {
		str[0] = '%';
	}
	if (str[str.length() - 1] == '_') {
		str[str.length() - 1] = '%';
	}
	return removeWhiteSpace(str);
}

string appendPatternClause(string clause, string column, string value) {
	if (value != "_") {
		clause = appendAnd(clause);
		if (isExactMatch(value)) {
			clause += "pattern_table." + column + " = '" + removeWhiteSpace(value) + "'";
		}
		else {
			value = checkAndReplaceLike(value);
			clause += "pattern_table." + column + " LIKE '" + value + "'";
		}
	}
	return clause;
}

string appendMainClause(string clause, string mainSynonymType) {
	if (mainSynonymType == "procedure") {
		clause += "procedure.procedureName FROM procedure ";
	}
	else if (mainSynonymType == "variable") {
		clause += "variable.name FROM variable ";
	}
	else if (mainSynonymType == "constant") {
		clause += "constant.value FROM constant ";
	}
	else if (isValInVectTwo({ "assign", "print", "read", "stmt", "while", "if" }, mainSynonymType)) {
		clause += mainSynonymType + ".stmtNo FROM " + mainSynonymType + " ";
	}
	return clause;
}

string formatTableName(string tableName) {
	if (tableName == "if") {
		return "if_table";
	}
	else if (tableName == "pattern") {
		return "pattern_table";
	}
	else if (isupper(tableName[0])) {
		tableName[0] = tolower(tableName[0]);
		return tableName;
	}
	return tableName;
}

// appends the join clause
string appendJoinClause(string clause, string targetTable, string mainSynonymType, vector<string> joinedTables) {
	targetTable = formatTableName(targetTable);
	mainSynonymType = formatTableName(mainSynonymType);
	// these tables return statement number
	if (!isValInVectTwo(joinedTables, targetTable) && isValInVectTwo({ "uses", "modifies", "pattern_table"}, targetTable)) {
		if (isValInVectTwo({"read", "print", "assign", "stmt", "while", "if_table"}, mainSynonymType)) {
			clause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".stmtNo = " + targetTable + ".stmtNo";
		}
		else if (mainSynonymType == "procedure") {
			clause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".name = " + targetTable + ".procedureName";
		}
		else if (mainSynonymType == "variable") {
			clause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".name = " + targetTable + ".target";
		}
		else if (mainSynonymType == "constant") {
			// constant never used, digit referenced is always stmtno in clauses
			throw invalid_argument("unexpected constant type tgt w modifies / uses / pattern.");
		}
		else {
			throw invalid_argument("unexpected synonym type: " + mainSynonymType);
		}
		joinedTables.push_back(targetTable);
	}
	return clause;
}

string appendWhereClause(string clause, string targetTable, string mainSynonymType, string source, string target, map<string, string> declarationMap) {
	targetTable = formatTableName(targetTable);
	mainSynonymType = formatTableName(mainSynonymType);
	// all synonym type except constant
	if (isValInVectTwo({ "stmt", "read", "print", "assign", "while", "if_table", "variable", "procedure" }, mainSynonymType)) {
		if (isdigit(source[0])) {
			clause = appendAnd(clause);
			clause += targetTable + ".stmtNo = " + "'" + source + "'";
		}
		else {
			if (targetTable == "pattern_table") {
				clause = appendPatternClause(clause, "source", source);
				clause = appendPatternClause(clause, "target", target);
			}
			else {
				if (isValInMap(declarationMap, source) && declarationMap[source] == "procedure") {
					clause = appendAnd(clause);
					clause += targetTable + ".procedureName = '" + source + "'";
				}
				if (isValInVectTwo({ "uses", "modifies" }, targetTable)) {
					if (isdigit(source[0])) {
						clause = appendAnd(clause);
						clause += targetTable + ".stmtNo = '" + source + "'";
					}
					clause = appendAnd(clause);
					clause += targetTable + ".target = '" + target + "'";
				}
			}
		}
	}
	return clause;
}

string checkExactMatch(int offset, vector<string> tokens) {
	string value = tokens.at(offset);
	// check for "variable name" (exact match)
	if (value == "\"") {
		value = "";
		offset += 1;
		if (offset < tokens.size()) {
			string tempToken = tokens.at(offset);
			while (tempToken != "\"") {
				value += tempToken;
				offset += 1;
				tempToken = tokens.at(offset);
			}
		}
	}
	return value;
}

string checkExactOrPartialMatch(int offset, vector<string> tokens) {
	// check for "variable name" (exact match)
	string value = checkExactMatch(offset, tokens);
	// check for _"variable"_ (partial match)
	if (value == "_" && (offset + 1) < (tokens.size() - 1) && tokens.at(offset + 1) == "\"") {
		offset += 2;
		string tempToken = tokens.at(offset);
		while (tempToken != "\"") {
			value += tempToken;
			offset += 1;
			tempToken = tokens.at(offset);
		}
		if (tokens.at(offset + 1) == "_") {
			value += "_";
		}
	}
	return value;
}

// method to evaluate a query
// This method currently only handles queries for getting all the procedure names,
// using some highly simplified logic.
// You should modify this method to complete the logic for handling all required queries.
void QueryProcessor::evaluate(string query, vector<string>& output) {
	// clear the output vector
	output.clear();

	// tokenize the query
	Tokenizer tk;
	vector<string> tokens;
	tk.tokenize(query, tokens);

	// create a vector for storing the results from database
	vector<string> databaseResults;

	map<string, string> declarationMap;
	vector<string> joinedTables;
	bool isEndOfDeclaration = false;
	// init synonym as tokens.at(0) as it is possible the expected column is not defined as a variable based on example
	string mainSynonymType = tokens.at(0);
	bool isInCondition = false;
	string joinClause = "";
	string whereClause = "";
	for (size_t i = 0; i < tokens.size(); i += 1) {
		string currToken = tokens.at(i);
		if (isInCondition) {
			if (currToken == ")") {
				// end of condition logic has to been improved to consider patterns with brackets
				isInCondition = false;
			}
			continue;
		} else if (isEndOfDeclaration) {
			// Start parsing queries
			if (currToken == "Parent") {
				isInCondition = true;
			}
			else if (currToken == "Parent*") {
				isInCondition = true;
			}
			else if (currToken == "Next") {
				isInCondition = true;
			}
			else if (currToken == "Next*") {
				isInCondition = true;
			}
			else if (isValInVectTwo({ "Uses", "Modifies" }, currToken)) {
				isInCondition = true;
				string source = tokens.at(i + 2);
				string target = tokens.at(i + 5);
				joinClause = appendJoinClause(joinClause, currToken, mainSynonymType, joinedTables);
				whereClause = appendWhereClause(whereClause, currToken, mainSynonymType, source, target, declarationMap);
			}
			else if (currToken == "pattern") {
				isInCondition = true;
				int offset = i + 3;
				// source can be _ (match all) or "variable name" (exact match)
				string source = checkExactMatch(offset, tokens);
				if (source[0] != '_') {
					offset += 4;
				}
				else {
					offset += 2;
				}
				// target can be _ (match all) or "variable name" (exact match) or _"variable"_ (partial match)
				string target = checkExactOrPartialMatch(offset, tokens);
				joinClause = appendJoinClause(joinClause, currToken, mainSynonymType, joinedTables);
				whereClause = appendWhereClause(whereClause, currToken, mainSynonymType, source, target, declarationMap);
			}
		} else if (currToken == ";") {
			// new declaration, insert as { name : synonymType }
			declarationMap.insert({ tokens.at(i - 1), tokens.at(i - 2) });
		} else if (currToken == "Select") {
			// Map the expected column if exists, else use default
			if (isValInMap(declarationMap, tokens.at(i + 1))) {
				mainSynonymType = declarationMap.at(tokens.at(i + 1));
			}
			// End of declaration, start parsing query
			isEndOfDeclaration = true;
		}
	}

	// call the method in database to retrieve the results
	// This logic is highly simplified based on iteration 1 requirements and 
	// the assumption that the queries are valid.
	string queryToExecute = "SELECT ";
	queryToExecute = appendMainClause(queryToExecute, mainSynonymType);
	if (joinClause != "") {
		queryToExecute += joinClause;
	}
	if (whereClause != "") {
		queryToExecute += " WHERE " + whereClause;
	}
	cout << "Query to be executed: " << queryToExecute << endl;
	Database::getQueryResults(databaseResults, queryToExecute);
	// post process the results to fill in the output vector
	for (string databaseResult : databaseResults) {
		output.push_back(databaseResult);
	}
}