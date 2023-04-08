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

string appendComma(string clause) {
	if (clause != "") {
		return clause + ", ";
	}
	return clause;
}

bool checkIfIsDigitForClause(string source) {
	if (source[0] == '\'' && isdigit(source[1])) {
		return true;
	}
	return false;
}

string removeQuotesFromInt(string source) {
	return source.substr(1, source.size() - 2);
}

string toLowerCase(string val) {
	//Apply tolower to each character of string
	std::transform(val.begin(), val.end(), val.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return val;
}

// method to check if value is found in the vector
bool isValInVectTwo(vector<string> vector, string value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

bool isVectinVect(vector<string> v1, vector<string> v2) {
	for (int i = 0; i < v2.size(); i += 1) {
		if (isValInVectTwo(v1, v2[i])) {
			return true;
		}
	}
	return false;
}

bool isValInMap(map<string, string> m, string value) {
	if (m.count(value)) {
		return true;
	}
	return false;
}

vector<string> addAllSynonym(vector<string> v1, vector<string> v2, map<string, string> declarationMap) {
	for (int i = 0; i < v2.size(); i += 1) {
		if (!isValInVectTwo(v1, v2[i]) && isValInMap(declarationMap, v2[i])) {
			v1.push_back(v2[i]);
		}
	}
	return v1;
}

bool isCharInVectTwo(vector<char> vector, char value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

bool isExactMatch(string str) {
	if (str.find('_') < str.length()) {
		return false;
	}
	return true;
}

bool isDirect(string targetTable) {
	if (isValInVectTwo({ "next*", "parent*", "calls*" }, targetTable)) {
		return false;
	}
	return true;
}

string removeWhiteSpace(string str) {
	str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
	return str;
}

string checkAndReplaceLike(string str) {
	if (str.size() > 2 && str[0] == '\'' && str[1] == '_') {
		str[1] = '%';
	}
	if (str.size() > 2 && str[str.length() - 1] == '\'' && str[str.length() - 2] == '_') {
		str[str.length() - 2] = '%';
	}
	str = removeWhiteSpace(str);
	string temp = "";
	for (int i = 1; i < str.size() - 1; i += 1) {
		if (str[i] == '%') {
			if (temp != "") {
				temp += "|";
			}
			temp += str[i];
			if (temp == "%") {
				temp += "|";
			}
		}
		else {
			if (isCharInVectTwo({ '(', ')', '>', '<', '+', '-', '*', '/', '%' }, str[i])) {
				if (temp != "%|") {
					temp += "||";
				}
				temp += str[i];
				temp += "||";
			}
			else {
				temp += str[i];
			}
		}
	}
	temp = "'" + temp + "'";
	return temp;
}

string checkAndAddDirect(string whereClause, string targetTableAlias, int i, vector<int> mainRefIndex, vector<pair<string, vector<string>>> typeToArgList, map<string, string> declarationMap) {
	if (
		isValInVectTwo({ "parent", "calls", "next" }, typeToArgList[mainRefIndex[i]].first) &&
		(
			typeToArgList[mainRefIndex[i]].second[0] != "'_'" &&
			!isValInMap(declarationMap, typeToArgList[mainRefIndex[i]].second[0])
		) &&
		(
			!isValInMap(declarationMap, typeToArgList[mainRefIndex[i]].second[1]) ||
			(
				// if RHS of clause is a synonym that is not while or if or stmt
				isValInMap(declarationMap, typeToArgList[mainRefIndex[i]].second[1]) &&
				!isValInVectTwo({ "while", "if_table", "stmt" }, declarationMap[typeToArgList[mainRefIndex[i]].second[1]])
				)
			)
		) {
		whereClause = appendAnd(whereClause);
		whereClause += targetTableAlias + ".direct = '1'";
	}
	return whereClause;
}

string appendPatternClause(string clause, string tableName, string column, string value) {
	if (value != "'_'") {
		clause = appendAnd(clause);
		if (isExactMatch(value)) {
			clause += tableName + "." + column + " = " + removeWhiteSpace(value);
		}
		else {
			value = checkAndReplaceLike(value);
			clause += tableName + "." + column + " LIKE " + value;
		}
	}
	return clause;
}

string appendMainClause(string clause, vector<string> mainSynonymVars, map<string, string> declarationMap) {
	string columnClause = "";
	string fromClause = "";
	for (int i = 0; i < mainSynonymVars.size(); i += 1) {
		string mainSynonymType = declarationMap[mainSynonymVars[i]];
		string tableName = mainSynonymType + "_" + mainSynonymVars[i];
		columnClause = appendComma(columnClause);
		fromClause = appendComma(fromClause);
		columnClause += tableName;
		fromClause += mainSynonymType + " AS " + tableName;
		if (mainSynonymType == "procedure") {
			columnClause += ".procedureName";
		}
		else if (mainSynonymType == "variable") {
			columnClause += ".name";
		}
		else if (mainSynonymType == "constant") {
			columnClause += ".value";
		}
		else if (isValInVectTwo({ "assign", "print", "read", "stmt", "while", "if_table", "calls" }, mainSynonymType)) {
			columnClause += ".stmtNo";
		}
	}
	return clause + columnClause + " FROM " + fromClause;
}

string formatTableName(string tableName) {
	tableName = toLowerCase(tableName);
	if (tableName == "if") {
		return "if_table";
	}
	else if (tableName == "pattern") {
		return "pattern_table";
	}
	else if (tableName == "parent" || tableName == "parent*") {
		return "parents";
	}
	else if (tableName == "next" || tableName == "next*") {
		return "nexts";
	}
	else if (tableName == "call" || tableName == "calls" || tableName == "calls*") {
		return "calls";
	}
	return tableName;
}

string appendWhereClause(string clause, string targetTable, string targetTableAlias, vector<string> mainSynonymTypes, string source, string target, map<string, string> declarationMap, bool direct) {
	for (int i = 0; i < mainSynonymTypes.size(); i += 1) {
		string mainSynonymType = mainSynonymTypes[i];
		if (isValInVectTwo({ "constant", "stmt", "read", "print", "assign", "while", "if_table", "variable", "procedure", "calls" }, mainSynonymType)) {
			if (checkIfIsDigitForClause(source) && !isValInVectTwo({ "parents", "nexts" }, targetTable)) {
				clause = appendAnd(clause);
				clause += targetTableAlias + ".stmtNo = " + source;
			}
			else {
				if (targetTable == "pattern_table") {
					if (!isValInMap(declarationMap, source)) {
						clause = appendPatternClause(clause, targetTableAlias, "source", source);
					}
					clause = appendPatternClause(clause, targetTableAlias, "target", target);
				}
				else {
					if (
						!isValInMap(declarationMap, source) &&
						mainSynonymType == "procedure" &&
						source != "'_'"
					) {
						clause = appendAnd(clause);
						clause += targetTableAlias + ".procedureName = " + source;
					}
					if (isValInVectTwo({ "uses", "modifies", "calls"}, targetTable)) {
						if (source.size() > 2 && checkIfIsDigitForClause(source)) {
							clause = appendAnd(clause);
							clause += targetTableAlias + ".stmtNo = " + source;
						}
						else if (
							!isValInMap(declarationMap, source) &&
							mainSynonymType != "procedure" &&
							source != "'_'"
						) {
							clause = appendAnd(clause);
							clause += targetTableAlias + ".procedureName = " + source;
						}
						if (!isValInMap(declarationMap, target) && target != "'_'") {
							string targetColumn = targetTable == "calls" ? ".targetProc" : ".target";
							clause = appendAnd(clause);
							clause += targetTableAlias + targetColumn + " = " + target;
						}
					}
				}
			}
			if (isValInVectTwo({ "parents", "nexts" }, targetTable)) {
				if (checkIfIsDigitForClause(source) || (isValInMap(declarationMap, source) && declarationMap[source] == "stmt")) {
					if (checkIfIsDigitForClause(source)) {
						clause = appendAnd(clause);
						string sourceColumnWithAlias = targetTableAlias + ".parentStmtNo";
						string op = " = ";
						if (targetTable == "nexts") {
							sourceColumnWithAlias = targetTableAlias + ".prevStmtNo";
							if (!direct) {
								op = " > ";
								sourceColumnWithAlias = "CAST(" + targetTableAlias + ".prevStmtNo" + " AS INT)";
								source = removeQuotesFromInt(source);
							}
						}
						clause += sourceColumnWithAlias + op + source;
					}
					if (targetTable == "parents") {
						clause = appendAnd(clause);
						clause += targetTableAlias + ".stmtNo != " + targetTableAlias + ".parentStmtNo";
					}
				}
				if (checkIfIsDigitForClause(target)) {
					clause = appendAnd(clause);
					string op = " = ";
					string targetColumnWithAlias = targetTableAlias + ".stmtNo";
					if (targetTable == "nexts" && !direct) {
						op = " < ";
						targetColumnWithAlias = "CAST(" + targetTableAlias + ".stmtNo" + " AS INT)";
						target = removeQuotesFromInt(target);
					}
					clause += targetColumnWithAlias + op + target;
				}

				if (direct) {
					clause = appendAnd(clause);
					clause += targetTableAlias + ".direct = '1'";
				}
				else if (
					isValInMap(declarationMap, target) &&
					targetTable == "parents" &&
					isValInVectTwo({ "while", "if_table" }, declarationMap[target])
				) {
					clause = appendAnd(clause);
					clause += targetTableAlias + ".isFirst = '0'";
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
		value = "'";
		offset += 1;
		if (offset < tokens.size()) {
			string tempToken = tokens.at(offset);
			while (tempToken != "\"") {
				value += tempToken;
				offset += 1;
				tempToken = tokens.at(offset);
			}
		}
		value += "'";
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
	value = "'" + value + "'";
	return value;
}

string appendJoinOnClause(string joinClause, vector<string> mainSynonymTypes, string source, string target, string patternRef, string targetTable, string targetTableAlias, string sourceTable, string sourceTableAlias, string mainSource, string mainTarget, map<string, string> declarationMap) {
	joinClause += " INNER JOIN " + targetTable + " AS " + targetTableAlias;
	string tempJoinClause = "";
	for (int i = 0; i < mainSynonymTypes.size(); i += 1) {
		string mainSynonymType = mainSynonymTypes[i];
		cout << "target: " << targetTable << endl;
		cout << "source: " << sourceTable << endl;
		if (sourceTable == "nexts" && targetTable == "nexts") {
			tempJoinClause += sourceTableAlias + ".prevStmtNo = " + targetTableAlias + ".prevStmtNo";

		}
		else if (targetTable == "procedure" || sourceTable == "procedure") {
			tempJoinClause = appendAnd(tempJoinClause);
			string joinColumn = ".procedureName";
			if (targetTable == "calls" && isValInMap(declarationMap, target)) {
				joinColumn = ".targetProc";
			}
			tempJoinClause += sourceTableAlias + ".procedureName = " + targetTableAlias + joinColumn;
		}
		else if (
			isValInVectTwo({ "read", "print", "assign", "stmt", "while", "if_table", "constant", "parents", "nexts", "calls" }, sourceTable)
		) {
			string sourceColumn = ".stmtNo";
			string joinColumn = ".stmtNo";
			if (isValInVectTwo({ "parents", "nexts" }, sourceTable) && (isValInMap(declarationMap, target) && !isValInVectTwo({"while", "if_table"}, declarationMap[target]))) {
				if (isValInMap(declarationMap, target) && isValInVectTwo({ "stmt" }, declarationMap[target])) {
					sourceColumn = sourceTable == "parents" ? ".parentStmtNo" : ".prevStmtNo";
				}
				if (
					isValInMap(declarationMap, target) &&
					mainSynonymType == declarationMap[target] &&
					// need to add condition to check if target's value relation matches the mainSynonymType
					isValInVectTwo({ "while", "if_table" }, declarationMap[target]) &&
					isValInVectTwo({ "while", "if_table" }, mainSynonymType)
				) {
					joinColumn = ".parentStmtNo";
				}
			}
			tempJoinClause = appendAnd(tempJoinClause);
			tempJoinClause += sourceTableAlias + sourceColumn + " = " + targetTableAlias + joinColumn;
		}
		else if (isValInVectTwo({ "read", "print", "assign", "stmt", "while", "if_table", "constant", "parents" }, targetTable)) {
			tempJoinClause = appendAnd(tempJoinClause);
			tempJoinClause += sourceTableAlias + ".stmtNo = " + targetTableAlias + ".stmtNo";
		}
		else if (isValInVectTwo({ "modifies", "uses", "pattern_table" }, sourceTable)) {
			string sourceColumn = ".stmtNo";
			string targetColumn = ".stmtNo";
			if (mainSynonymType == "variable") {
				if (targetTable == "pattern_table") {
					if (mainSource != patternRef) {
						sourceColumn = ".target";
						if (sourceTable == "modifies") {
							targetColumn = ".source";
						}
					}
				}
				else {
					if (sourceTable == "modifies" || sourceTable == "uses") {
						sourceColumn = ".target";
					}
					else {
						sourceColumn = ".source";
					}
					targetColumn = ".name";
				}
			}
			else if (sourceTable == "pattern_table" && (targetTable == "modifies" || targetTable == "uses")) {
				sourceColumn = ".source";
				targetColumn = ".target";
			}
			tempJoinClause = appendAnd(tempJoinClause);
			tempJoinClause += sourceTableAlias + sourceColumn + " = " + targetTableAlias + targetColumn;
		}
		else if (sourceTable == "variable") {
			tempJoinClause = appendAnd(tempJoinClause);
			tempJoinClause += sourceTableAlias + ".name = " + targetTableAlias;
			if (targetTable == "pattern_table") {
				tempJoinClause += ".source";
			}
			else {
				tempJoinClause += ".target";
			}
		}
	}
	if (tempJoinClause != "") {
		joinClause += " ON " + tempJoinClause;
	}
	return joinClause;
}

string appendEndOfNestedJoin(string joinClause, vector<string> mainSynonymTypes, string mainSource, vector<string> mainSynonymVars, vector<int> mainRefIndex, vector<pair<string, vector<string>>> typeToArgList, map<string, string> declarationMap) {
	string tempJoinClause = "";
	for (int i = 0; i < mainSynonymVars.size(); i += 1) {
		tempJoinClause = appendAnd(tempJoinClause);
		string tableName = declarationMap[mainSynonymVars[i]];
		string tableAlias = tableName + "_" + mainSynonymVars[i];
		tempJoinClause += tableAlias;
		if (tableName == "variable") {
			tempJoinClause += ".name = MAIN_REF_TABLE";
			if (isValInVectTwo({ "modifies", "uses" }, formatTableName(typeToArgList[mainRefIndex[0]].first))) {
				tempJoinClause += ".target";
			}
			else {
				tempJoinClause += ".source";
			}
		}
		else if (tableName == "procedure") {
			tempJoinClause += ".procedureName = MAIN_REF_TABLE.procedureName";
		}
		else {
			string joinColumn = ".stmtNo";
			if (
				isValInMap(declarationMap, mainSource) &&
				formatTableName(typeToArgList[mainRefIndex[0]].first) == "parents" &&
				mainSource == mainSynonymVars[i]
			) {
				joinColumn = ".parentStmtNo";
			}
			tempJoinClause += ".stmtNo = MAIN_REF_TABLE" + joinColumn;
		}
	}
	if (tempJoinClause != "") {
		joinClause += ") as MAIN_REF_TABLE ON " + tempJoinClause;
	}
	return joinClause;
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

	map<string, string> declarationMap; // { name : synonymType }
	vector<pair<string, vector<string>>> typeToArgList; // vector : [ synonymType : { source : target }]
	vector<string> joinedTables;
	bool isEndOfDeclaration = false;
	bool isEndOfMultiSynonymDeclaration = true;
	vector<string> mainSynonymTypes;
	vector<string> mainSynonymVars;
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
		else if (!isEndOfMultiSynonymDeclaration) {
			if (currToken == ">") {
				isEndOfMultiSynonymDeclaration = true;
			}
		}
		else if (isEndOfDeclaration) {
			// Start parsing queries
			if (isValInVectTwo({ "parent", "next" }, toLowerCase(currToken))) {
				isInCondition = true;
				int offset = 2;
				if (tokens.at(i + 1) == "*") {
					offset = 3;
					currToken += "*";
				}
				string source = tokens.at(i + offset);
				if (isdigit(source[0]) || source[0] == '_') {
					source = "'" + source + "'";
				}
				string target = tokens.at(i + offset + 2);
				if (isdigit(target[0]) || target[0] == '_') {
					target = "'" + target + "'";
				}
				// parent type always insert at front of typeToArgMap
				typeToArgList.insert(typeToArgList.begin(), { toLowerCase(currToken), { source, target } });
			}
			else if (isValInVectTwo({ "uses", "modifies", "calls"}, toLowerCase(currToken))) {
				isInCondition = true;
				int offset = 2;
				if (tokens.at(i + 1) == "*") {
					offset = 3;
					currToken += "*";
				}
				string source = tokens.at(i + offset);
				if (source == "\"") {
					offset += 1;
					source = tokens.at(i + offset);
					source = "'" + source + "'";
					offset += 1;
				}
				else if (isdigit(source[0])) {
					source = "'" + source + "'";
				}
				offset += 3;
				string target = tokens.at(i + offset);
				if (tokens.at(i + offset - 1)[0] != '"') {
					target = tokens.at(i + offset - 1);
					if (isdigit(target[0]) || target == "_") {
						target = "'" + target + "'";
					}
				}
				else {
					target = "'" + target + "'";
				}
				typeToArgList.push_back({ toLowerCase(currToken), { source, target } });
			}
			else if (toLowerCase(currToken) == "pattern") {
				isInCondition = true;
				int offset = i + 3;
				string patternRef = tokens.at(i + 1);
				// source can be _ (match all) or "variable name" (exact match) or synonymTypeVar
				string source = checkExactMatch(offset, tokens);
				if (isdigit(source[0]) || source == "_") {
					source = "'" + source + "'";
				}
				if (source[0] != '_' && tokens.at(i + 3) == "\"") {
					offset += 4;
				}
				else {
					offset += 2;
				}
				// target can be _ (match all) or "variable name" (exact match) or _"variable"_ (partial match)
				string target = checkExactOrPartialMatch(offset, tokens);
				typeToArgList.push_back({ toLowerCase(currToken), { source, target, patternRef } });
			}
		}
		else if (currToken == ";" || currToken == ",") {
			string synonymType = formatTableName(tokens.at(i - 2));
			if (synonymType == ",") {
				synonymType = declarationMap[tokens.at(i - 3)];
			}
			// new declaration, insert as { name : { synonymType : synonymTypeAlias } }
			declarationMap.insert({ tokens.at(i - 1), synonymType });
		}
		else if (currToken == "Select") {
			// Multiple Synonyms to be selected
			if (tokens.at(i + 1) == "<") {
				isEndOfMultiSynonymDeclaration = false;
				int offset = i + 2;
				string tempToken = tokens.at(offset);
				while (tempToken != ">") {
					if (tempToken != ",") {
						string formattedTableName = formatTableName(declarationMap.at(tempToken));
						mainSynonymTypes.push_back(formattedTableName);
						joinedTables.push_back(formattedTableName);
						mainSynonymVars.push_back(tempToken);
					}
					offset += 1;
					tempToken = tokens.at(offset);
				}
			}
			// Map the expected column if exists, else use default
			else if (isValInMap(declarationMap, tokens.at(i + 1))) {
				mainSynonymTypes.push_back(formatTableName(declarationMap.at(tokens.at(i + 1))));
				mainSynonymVars.push_back(tokens.at(i + 1));
				joinedTables.push_back(mainSynonymTypes[0]);
			}
			else {
				// init synonym as tokens.at(0) as it is possible the expected column is not defined as a variable based on example
				mainSynonymTypes.push_back(formatTableName(tokens.at(0)));
				mainSynonymVars.push_back(tokens.at(0));
			}
			// End of declaration, start parsing query
			isEndOfDeclaration = true;
		}
	}

	string joinClause = "";
	string whereClause = "";
	vector<string> mappedIndex; // track for typeToArgMap already used
	vector<int> mainRefIndex; // track index of clauses that are related to main synonym
	vector<string> mainRefSynonym; // track synonyms of clauses that are related to main synonym, directly or indirectly
	vector<string> joinedSynonymVar; // track synonym already joined
	// track all the main synonym var being used (needed for multiple synonyms)
	mainRefSynonym.insert(mainRefSynonym.end(),mainSynonymVars.begin(), mainSynonymVars.end());
	// map all related clauses
	bool loopAgain = true;
	bool hasIfOrWhileOrStmtInParent = false;
	// loop through all the clauses to add all related clauses to mainRefIndex
	// loopAgain retriggers if at least 1 clause is added, so that all related synonym to the added clause is added as well
	while (loopAgain) {
		loopAgain = false;
		for (int j = 0; j < typeToArgList.size(); j += 1) {
			// check if current index is already added
			if (!isValInVectTwo(mappedIndex, to_string(j))) {
				// if any synonym matches any of the main synonym
				if (isVectinVect(typeToArgList[j].second, mainRefSynonym)) {
					if (
						formatTableName(typeToArgList[j].first) == "parents" &&
						(
							// check if either LHS or RHS of the parents clause is while / if / stmt synonym
							(isValInMap(declarationMap, typeToArgList[j].second[0]) && isValInVectTwo({ "while", "if_table", "stmt" }, declarationMap[typeToArgList[j].second[0]])) ||
							(isValInMap(declarationMap, typeToArgList[j].second[1]) && isValInVectTwo({ "while", "if_table", "stmt" }, declarationMap[typeToArgList[j].second[1]]))
						)
					) {
						// hasIfOrWhileOrStmtInParent will trigger nested join logic even if there is only 1 relevant clause
						hasIfOrWhileOrStmtInParent = true;
					}
					mainRefIndex.push_back(j);
					mappedIndex.push_back(to_string(j));
					mainRefSynonym = addAllSynonym(mainRefSynonym, typeToArgList[j].second, declarationMap);
					loopAgain = true;
				}
			}
		}
	}
	// ------------------------------------- ONLY 1 RELEVANT CLAUSE AND NOT HAVE WHILE / IF IN PARENT JOIN NORMALLY -------------------------------------
	if (mainRefIndex.size() == 1 && !hasIfOrWhileOrStmtInParent) {
		string targetTable = formatTableName(typeToArgList[mainRefIndex[0]].first);
		string unformattedTargetTable = typeToArgList[mainRefIndex[0]].first;
		string source = typeToArgList[mainRefIndex[0]].second[0];
		string sourceTableAlias = isValInMap(declarationMap, source) ? declarationMap[source] : "";
		string target = typeToArgList[mainRefIndex[0]].second[1];
		string targetTableAlias = isValInMap(declarationMap, target) ? declarationMap[target] : "";
		string patternRef = "";
		// patternRef refers to the var for assign used for the pattern
		// In the example below, a is the patternRef
		// assign a; variable v;
		// SELECT v such that pattern a (v, "b") ..."
		if (typeToArgList[mainRefIndex[0]].second.size() > 2) {
			patternRef = typeToArgList[mainRefIndex[0]].second[2];
		}
		// only 1 relevant clauses, join target table normally
		// eg. Select a such that Uses (a, v)
		for (int i = 0; i < mainSynonymVars.size(); i += 1) {
			string tempSynonymTable = declarationMap[mainSynonymVars[i]];
			string tempSynonymAlias = tempSynonymTable + "_" + mainSynonymVars[i];
			// Join the clause table
			joinClause = appendJoinOnClause(
				joinClause, mainSynonymTypes,
				source, target, patternRef,
				targetTable, targetTable,
				tempSynonymTable, tempSynonymAlias,
				source, target, declarationMap
			);
			// join LHS of target table arg if not already joined
			if (
				isValInMap(declarationMap, source) &&
				!isValInVectTwo(mainSynonymTypes, declarationMap[source]) &&
				!isValInVectTwo(joinedSynonymVar, source)
			) {
				joinClause = appendJoinOnClause(
					joinClause, mainSynonymTypes,
					source, target, patternRef,
					declarationMap[source], sourceTableAlias,
					tempSynonymTable, tempSynonymAlias,
					source, target, declarationMap
				);
				joinedSynonymVar.push_back(source);
			}
			// join RHS of target table arg if not already joined
			if (
				isValInMap(declarationMap, target) &&
				!isValInVectTwo(mainSynonymTypes, declarationMap[target]) &&
				!isValInVectTwo(joinedSynonymVar, source)
			) {
				joinClause = appendJoinOnClause(
					joinClause, mainSynonymTypes,
					source, target, patternRef,
					declarationMap[target], targetTableAlias,
					tempSynonymTable, tempSynonymAlias,
					source, target, declarationMap
				);
				joinedSynonymVar.push_back(target);
			}
			bool direct = isDirect(unformattedTargetTable);
			// Append where clause
			whereClause = appendWhereClause(whereClause, targetTable, targetTable, mainSynonymTypes, source, target, declarationMap, direct);
			whereClause = checkAndAddDirect(whereClause, targetTable, 0, mainRefIndex, typeToArgList, declarationMap);
		}
	}
	// ------------------------------------- AT LEAST 1 RELEVANT CLAUSE, NEST JOINS -------------------------------------
	else if (mainRefIndex.size() > 0) {
		// more than 1 relevant clauses, nest joins
		string mainTable = formatTableName(typeToArgList[mainRefIndex[0]].first); // the main clause table
		string mainSource = typeToArgList[mainRefIndex[0]].second[0]; // LHS variable of main clause
		string mainTarget = typeToArgList[mainRefIndex[0]].second[1]; // RHS variable of main clause
		string mainTableAlias = "t_1"; // the alias for table belonging the main clause
		string refWhereClause = "";
		// loops through all the relevant clauses
		for (int i = 0; i < mainRefIndex.size(); i += 1) {
			string targetTable = formatTableName(typeToArgList[mainRefIndex[i]].first); // table for this clause
			string unformattedTargetTable = typeToArgList[mainRefIndex[i]].first; // unformatted table name to check if parent or parent*
			string source = typeToArgList[mainRefIndex[i]].second[0]; // LHS variable of this clause
			string target = typeToArgList[mainRefIndex[i]].second[1]; // RHS variable of this clause
			string patternRef = "";
			// patternRef as explained in previous logic
			if (typeToArgList[mainRefIndex[i]].second.size() > 2) {
				patternRef = typeToArgList[mainRefIndex[i]].second[2];
			}
			string targetTableAlias = "t_" + to_string(i + 1); // the alias for table belonging the clause
			string refSourceAlias = source; //"t_source_" + to_string(i + 1); // the alias for table belonging to LHS of the clause
			string refTargetAlias = target; //"t_target_" + to_string(i + 1); // the alias for table belonging to RHS of the clause
			if (i == 0) {
				// append nested join if is the first loop
				joinClause += " INNER JOIN (SELECT * FROM " + mainTable + " AS " + mainTableAlias;
			}
			else {
				// else append inner join clause
				joinClause = appendJoinOnClause(
					joinClause, mainSynonymTypes,
					source, target,
					patternRef, targetTable, targetTableAlias,
					mainTable, mainTableAlias, mainSource, mainTarget,
					declarationMap
				);
			}
			// join LHS of target table arg if not already joined
			if (isValInMap(declarationMap, source) && !isValInVectTwo(joinedSynonymVar, source) && !isValInVectTwo({ mainTable }, declarationMap[source])) {
				joinClause = appendJoinOnClause(
					joinClause,
					mainSynonymTypes,
					source, target,
					patternRef, declarationMap[source],
					refSourceAlias, mainTable, mainTableAlias,
					mainSource, mainTarget,
					declarationMap
				);
				joinedSynonymVar.push_back(source);
			}
			// join RHS of target table arg if not already joined
			if (isValInMap(declarationMap, target) && !isValInVectTwo(joinedSynonymVar, target) && !isValInVectTwo({ mainTable }, declarationMap[target])) {
				joinClause = appendJoinOnClause(
					joinClause,
					mainSynonymTypes,
					source, target,
					patternRef, declarationMap[target],
					refTargetAlias, mainTable, mainTableAlias,
					mainSource, mainTarget,
					declarationMap);
				joinedSynonymVar.push_back(target);
			}
			bool targetDirect = isDirect(unformattedTargetTable);
			// Append where clause
			refWhereClause = appendWhereClause(
				refWhereClause,
				targetTable, targetTableAlias,
				mainSynonymTypes,
				source, target,
				declarationMap, targetDirect
			);
			if (
				isValInMap(declarationMap, source) &&
				isValInVectTwo({ "while", "if_table" }, declarationMap[target]) &&
				isValInMap(declarationMap, target) &&
				isValInVectTwo({ "while", "if_table" }, declarationMap[source])
			) {
				refWhereClause = appendAnd(refWhereClause);
				refWhereClause += "CAST(" + source + ".parentStmtNo as INT) < CAST(" + target + ".parentStmtNo as INT)";
			}
			refWhereClause = checkAndAddDirect(refWhereClause, targetTableAlias, i, mainRefIndex, typeToArgList, declarationMap);
		}
		if (refWhereClause != "") {
			joinClause += " WHERE " + refWhereClause;
		}
		// close off nested join
		joinClause = appendEndOfNestedJoin(joinClause, mainSynonymTypes, mainSource, mainSynonymVars, mainRefIndex, typeToArgList, declarationMap);
	}
	string irrelevantClause = "";
	// ------------------------------------- CHECK ALL REMAINING IRRELEVANT CLAUSES -------------------------------------
	for (int i = 0; i < typeToArgList.size(); i += 1) {
		if (isValInVectTwo(mappedIndex, to_string(i))) {
			// skipped index that are already mapped
			continue;
		}
		// currently do not support related irrelevant clauses
		string targetTable = formatTableName(typeToArgList[i].first);
		string unformattedTargetTable = typeToArgList[i].first;
		string source = typeToArgList[i].second[0];
		string target = typeToArgList[i].second[1];
		string irrWhereClause = "";
		bool targetDirect = isDirect(unformattedTargetTable);
		irrelevantClause = appendAnd(irrelevantClause);
		irrelevantClause += "(SELECT COUNT(*) FROM " + targetTable;
		irrWhereClause = appendWhereClause(
			irrWhereClause,
			targetTable, targetTable,
			mainSynonymTypes,
			source, target,
			declarationMap, targetDirect
		);
		// irrelevantClause = checkAndAddDirect(irrelevantClause, targetTable, i, mainRefIndex, typeToArgList, declarationMap);
		if (irrWhereClause != "") {
			irrelevantClause += " WHERE " + irrWhereClause;
		}
		irrelevantClause += ") > 0";
		// for remaining clauses check related and check rows at least 1,
	}

	// call the method in database to retrieve the results
	// This logic is highly simplified based on iteration 1 requirements and 
	// the assumption that the queries are valid.
	string queryToExecute = "SELECT DISTINCT ";
	queryToExecute = appendMainClause(queryToExecute, mainSynonymVars, declarationMap);
	if (joinClause != "") {
		queryToExecute += joinClause;
	}
	for (int i = 0; i < mainSynonymVars.size(); i += 1) {
		if (isValInVectTwo({ "while", "if_table" }, declarationMap[mainSynonymVars[i]])) {
			whereClause = appendAnd(whereClause);
			whereClause += declarationMap[mainSynonymVars[i]] + "_" + mainSynonymVars[i] + ".isParent = '1'";
		}
	}
	if (whereClause != "") {
		queryToExecute += " WHERE " + whereClause;
		if (irrelevantClause != "") {
			queryToExecute += " AND " + irrelevantClause;
		}
	}
	else if (irrelevantClause != "") {
		queryToExecute += " WHERE " + irrelevantClause;
	}
	cout << "Query to be executed: " << queryToExecute << endl;
	Database::getQueryResults(databaseResults, queryToExecute);
	// post process the results to fill in the output vector
	for (string databaseResult : databaseResults) {
		output.push_back(databaseResult);
	}
}