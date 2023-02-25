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

bool isValInMap(map<string, pair<string, string>> m, string value) {
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

bool isDirect(string targetTable) {
	if (isValInVectTwo({ "Next*","Parent*" }, targetTable)) {
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
	else if (isValInVectTwo({ "assign", "print", "read", "stmt", "while", "if_table" }, mainSynonymType)) {
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
	else if (tableName == "Parent" || tableName == "Parent*") {
		return "parents";
	}
	else if (tableName == "Next" || tableName == "Next*") {
		return "nexts";
	}
	else if (isupper(tableName[0])) {
		tableName[0] = tolower(tableName[0]);
		return tableName;
	}
	return tableName;
}

bool isNotWhileOrIfOrParent(map<string, pair<string, string>> declarationMap, string value) {
	return isValInMap(declarationMap, value) && !isValInVectTwo({ "while", "if_table", "Parent", "Parent*", "if" }, declarationMap[value].first);
}

// appends the join clause
string appendJoinClause(
	string clause, string targetTable, string mainSynonymType, string source, string target, map<string,
	pair<string, string>> declarationMap, vector<string> parentDeclarationVect, vector<string> joinedTables
) {
	targetTable = formatTableName(targetTable);
	// these tables return statement number
	if (!isValInVectTwo(joinedTables, targetTable) && isValInVectTwo({ "uses", "modifies", "pattern_table", "parents", "nexts"}, targetTable)) {
		if (isValInVectTwo({"read", "print", "assign", "stmt", "while", "if_table", "constant" }, mainSynonymType)) {
			if (targetTable == "parents") {
				if (isValInVectTwo({ "while", "if_table" }, mainSynonymType) && (isNotWhileOrIfOrParent(declarationMap, source) || isNotWhileOrIfOrParent(declarationMap, target))) {
					clause += "INNER JOIN (SELECT * FROM " + targetTable;
					if (isNotWhileOrIfOrParent(declarationMap, source)) {
						clause += " INNER JOIN " + declarationMap[source].second + " ON parents.stmtNo = " + declarationMap[source].second + ".stmtNo";
						joinedTables.push_back(declarationMap[source].first);
						parentDeclarationVect.push_back(source);
					}
					if (isNotWhileOrIfOrParent(declarationMap, target)) {
						clause += " INNER JOIN " + declarationMap[target].second + " ON parents.stmtNo = " + declarationMap[target].second + ".stmtNo";
						joinedTables.push_back(declarationMap[target].first);
						parentDeclarationVect.push_back(target);
					}
					clause += ") as " + target + " ON " + mainSynonymType + ".stmtNo = " + target + ".parentStmtNo";
					declarationMap[target].second = target;
				}
				else {
					// check parents target and source type and join accordingly
					if (isValInMap(declarationMap, source) && !isValInVectTwo(joinedTables, declarationMap[source].first)) {
						clause += " INNER JOIN " + declarationMap[source].second + " ON " + mainSynonymType + ".stmtNo = " + declarationMap[source].second + ".stmtNo";
						joinedTables.push_back(declarationMap[source].first);
					}
					if (isValInMap(declarationMap, target) && !isValInVectTwo(joinedTables, declarationMap[target].first)) {
						clause += " INNER JOIN " + declarationMap[target].second + " ON " + mainSynonymType + ".stmtNo = " + declarationMap[target].second + ".stmtNo";
						joinedTables.push_back(declarationMap[target].first);
					}
					clause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".stmtNo = " + targetTable + ".stmtNo";
				}
			}
			else {
				clause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".stmtNo = " + targetTable + ".stmtNo";
			}
		}
		else if (mainSynonymType == "procedure") {
			clause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".name = " + targetTable + ".procedureName";
		}
		else if (mainSynonymType == "variable") {
			clause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".name = " + targetTable + ".target";
		}
		else {
			throw invalid_argument("unexpected synonym type: " + mainSynonymType);
		}
		joinedTables.push_back(targetTable);
	}
	return clause;
}

string appendWhereClause(string clause, string targetTable, string mainSynonymType, string source, string target, map<string, pair<string, string>> declarationMap) {
	bool direct = isDirect(targetTable);
	targetTable = formatTableName(targetTable);
	if (isValInVectTwo({ "stmt", "read", "print", "assign", "while", "if_table", "variable", "procedure" }, mainSynonymType)) {
		if (isdigit(source[0]) && targetTable != "parents") {
			clause = appendAnd(clause);
			clause += targetTable + ".stmtNo = " + "'" + source + "'";
		}
		else {
			if (targetTable == "pattern_table") {
				clause = appendPatternClause(clause, "source", source);
				clause = appendPatternClause(clause, "target", target);
			}
			else {
				if (isValInMap(declarationMap, source) && declarationMap[source].first == "procedure") {
					clause = appendAnd(clause);
					clause += targetTable + ".procedureName = '" + source + "'";
				}
				if (isValInVectTwo({ "uses", "modifies" }, targetTable)) {
					if (isdigit(source[0])) {
						clause = appendAnd(clause);
						clause += targetTable + ".stmtNo = '" + source + "'";
					}
					else if (!isValInMap(declarationMap, target)) {
						clause = appendAnd(clause);
						clause += targetTable + ".target = '" + target + "'";
					}
				}
			}
		}
		if (targetTable == "parents") {
			if (isdigit(source[0])) {
				clause = appendAnd(clause);
				clause += targetTable + ".parentStmtNo = '" + source + "'";
			}
			if (isdigit(target[0])) {
				clause = appendAnd(clause);
				clause += targetTable + ".parentStmtNo = '" + target + "'";
			}
			if (direct) {
				clause = appendAnd(clause);
				clause += targetTable + ".direct = '1'";
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

	map<string, pair<string, string>> declarationMap; // { name : { synonymType : synonymTypeAlias } }
	vector<pair<string, vector<string>>> typeToArgList; // vector : [ synonymType : { source : target }]
	vector<string> joinedTables;
	bool isEndOfDeclaration = false;
	// init synonym as tokens.at(0) as it is possible the expected column is not defined as a variable based on example
	string mainSynonymType = formatTableName(tokens.at(0));
	bool isInCondition = false;

	for (size_t i = 0; i < tokens.size(); i += 1) {
		string currToken = tokens.at(i);
		if (isInCondition) {
			if (currToken == ")") {
				// TODO: end of condition logic has to been improved to consider patterns with brackets
				isInCondition = false;
			}
			continue;
		}
		else if (isEndOfDeclaration) {
			// Start parsing queries
			if (currToken == "Parent") {
				int offset = 2;
				if (tokens.at(i + 1) == "*") {
					offset = 3;
					currToken += "*";
				}
				isInCondition = true;
				string source = tokens.at(i + offset);
				string target = tokens.at(i + offset + 2);
				// parent type always insert at front of typeToArgMap
				typeToArgList.insert(typeToArgList.begin(), { currToken, { source, target } });
			}
			else if (currToken == "Next") {
				isInCondition = true;
				string source = tokens.at(i + 2);
				string target = tokens.at(i + 4);
				typeToArgList.push_back({ currToken, { source, target } });
			}
			else if (currToken == "Next*") {
				isInCondition = true;
			}
			else if (isValInVectTwo({ "Uses", "Modifies" }, currToken)) {
				isInCondition = true;
				string source = tokens.at(i + 2);
				string target = tokens.at(i + 5);
				if (tokens.at(i + 4)[0] != '"') {
					target = tokens.at(i + 4);
				}
				typeToArgList.push_back({ currToken, { source, target } });
			}
			else if (currToken == "pattern") {
				isInCondition = true;
				int offset = i + 3;
				string patternRef = tokens.at(i + 1);
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
				typeToArgList.push_back({ currToken, { source, target, patternRef } });
			}
		}
		else if (currToken == ";") {
			// new declaration, insert as { name : { synonymType : synonymTypeAlias } }
			declarationMap.insert({ tokens.at(i - 1), { formatTableName(tokens.at(i - 2)), formatTableName(tokens.at(i - 2)) } });
		}
		else if (currToken == "Select") {
			// Map the expected column if exists, else use default
			if (isValInMap(declarationMap, tokens.at(i + 1))) {
				mainSynonymType = declarationMap.at(tokens.at(i + 1)).first;
			}
			joinedTables.push_back(mainSynonymType);
			// End of declaration, start parsing query
			isEndOfDeclaration = true;
		}
	}


	string joinClause = "";
	string whereClause = "";
	map<string, pair<string, string>> parentRefMap; // track for clauses that reference a variable in a parent
	vector<string> mappedIndex; // track for typeToArgMap already used
	for (int i = 0; i < typeToArgList.size(); i += 1) {
		if (isValInVectTwo(mappedIndex, to_string(i))) {
			// skipped index that are already mapped
			continue;
		}
		string targetTable = formatTableName(typeToArgList[i].first);
		string unformattedTargetTable = typeToArgList[i].first;
		string source = typeToArgList[i].second[0];
		string target = typeToArgList[i].second[1];
		if (targetTable == "parents") {
			mappedIndex.push_back(to_string(i));
			// find all clauses that are related to this parent clause
			vector<int> parentRefIndex;
			for (int j = 0; j < typeToArgList.size(); j += 1) {
				// not current index and not mapped
				if (j != i && !isValInVectTwo(mappedIndex, to_string(j)) && formatTableName(typeToArgList[j].first) != "parents") {
					for (string ref : typeToArgList[j].second) {
						if (ref == source || ref == target) {
							parentRefIndex.push_back(j);
							mappedIndex.push_back(to_string(j));
						}
					}
				}
			}
			if (!isValInVectTwo({ "while", "if_table" }, mainSynonymType) && parentRefIndex.size() == 0) {
				// check parents target and source type and join accordingly
				if (isValInMap(declarationMap, source) && !isValInVectTwo(joinedTables, declarationMap[source].first)) {
					joinClause += " INNER JOIN " + declarationMap[source].second + " ON " + mainSynonymType + ".stmtNo = " + declarationMap[source].second + ".stmtNo";
				}
				if (isValInMap(declarationMap, target) && !isValInVectTwo(joinedTables, declarationMap[target].first)) {
					joinClause += " INNER JOIN " + declarationMap[target].second + " ON " + mainSynonymType + ".stmtNo = " + declarationMap[target].second + ".stmtNo";
				}
				joinClause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".stmtNo = " + targetTable + ".stmtNo";
				whereClause = appendWhereClause(whereClause, unformattedTargetTable, mainSynonymType, source, target, declarationMap);
			}
			else {
				joinClause += "INNER JOIN (SELECT * FROM " + targetTable;
				if (isNotWhileOrIfOrParent(declarationMap, source)) {
					joinClause += " INNER JOIN " + declarationMap[source].second + " ON parents.stmtNo = " + declarationMap[source].second + ".stmtNo";
				}
				if (isNotWhileOrIfOrParent(declarationMap, target)) {
					joinClause += " INNER JOIN " + declarationMap[target].second + " ON parents.stmtNo = " + declarationMap[target].second + ".stmtNo";
				}
				string refWhereClause = "";
				for (int z = 0; z < parentRefIndex.size(); z += 1) {
					string refTable = formatTableName(typeToArgList[parentRefIndex[z]].first);
					string unformattedRefTable = typeToArgList[parentRefIndex[z]].first;
					string refSource = typeToArgList[parentRefIndex[z]].second[0];
					string refTarget = typeToArgList[parentRefIndex[z]].second[1];
					joinClause += " INNER JOIN " + refTable + " ON parents.stmtNo = " + refTable + ".stmtNo";
					refWhereClause = appendWhereClause(refWhereClause, unformattedRefTable, mainSynonymType, refSource, refTarget, declarationMap);
				}
				refWhereClause = appendWhereClause(refWhereClause, unformattedTargetTable, mainSynonymType, source, target, declarationMap);
				if (refWhereClause != "") {
					joinClause += " WHERE " + refWhereClause;
				}
				joinClause += ") as temp_" + target + source + " ON " + mainSynonymType + ".stmtNo = temp_" + target + source;
				if (!isValInVectTwo({ "while", "if_table" }, mainSynonymType) || parentRefIndex.size() == 0) {
					joinClause += ".stmtNo";
				}
				else {
					joinClause += ".parentStmtNo";
				}
			}
		}
		else if (isValInVectTwo({ "uses", "modifies", "pattern_table" }, targetTable)) {
			if (isValInVectTwo({ "read", "print", "assign", "stmt", "while", "if_table", "constant" }, mainSynonymType)) {
				joinClause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".stmtNo = " + targetTable + ".stmtNo";
			}
			else if (mainSynonymType == "procedure") {
				joinClause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".name = " + targetTable + ".procedureName";
			}
			else if (mainSynonymType == "variable") {
				joinClause += " INNER JOIN " + targetTable + " ON " + mainSynonymType + ".name = " + targetTable + ".target";
			}
			else {
				throw invalid_argument("unexpected synonym type: " + mainSynonymType);
			}
			whereClause = appendWhereClause(whereClause, unformattedTargetTable, mainSynonymType, source, target, declarationMap);
		}
	}

	// call the method in database to retrieve the results
	// This logic is highly simplified based on iteration 1 requirements and 
	// the assumption that the queries are valid.
	string queryToExecute = "SELECT DISTINCT ";
	queryToExecute = appendMainClause(queryToExecute, mainSynonymType);
	if (joinClause != "") {
		queryToExecute += joinClause;
	}
	if (isValInVectTwo({ "while", "if_table" }, mainSynonymType)) {
		whereClause = appendAnd(whereClause);
		whereClause += mainSynonymType + ".isParent = '1'";
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