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

string removeQuotes(string source) {
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

bool isDirect(string targetTable, string source, string target, map<string, string> declarationMap) {
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
		isValInVectTwo({ "parent", "next" }, typeToArgList[mainRefIndex[i]].first) &&
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
	else if (isValInVectTwo({ "calls" }, typeToArgList[mainRefIndex[i]].first)) {
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

string getColumnName(string type) {
	string column = "";
	if (type == "procedure") {
		column = ".procedureName";
	}
	else if (type == "variable") {
		column = ".name";
	}
	else if (type == "constant") {
		column = ".value";
	}
	else if (isValInVectTwo({ "assign", "print", "read", "stmt", "while", "if_table", "calls" }, type)) {
		column = ".stmtNo";
	}
	return column;
}

string getSubColumnClause(vector<string> clauseVars, vector<string> mainSynonymVars, map<string, string> declarationMap) {
	string columnClause = "";
	for (int i = 0; i < clauseVars.size(); i += 1) {
		if (!(isValInVectTwo(mainSynonymVars, clauseVars[i]) || isValInMap(declarationMap, clauseVars[i]))) {
			continue;
		}
		string mainSynonymType = declarationMap[clauseVars[i]];
		string tableName = "TABLE_" + clauseVars[i];
		columnClause = appendComma(columnClause);
		columnClause += tableName + getColumnName(mainSynonymType);
		columnClause += " AS " + clauseVars[i];
	}
	if (columnClause == "") {
		return "*";
	}
	return columnClause;
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
		columnClause += getColumnName(mainSynonymType);
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

string appendWhereClause(string clause, string targetTable, string targetTableAlias, vector<string> mainSynonymVars, vector<string> clauseVars, map<string, string> declarationMap, bool direct) {
	bool directAdded = false;
	bool isFirstAdded = false;
	for (int i = 0; i < mainSynonymVars.size(); i += 1) {
		if (!isValInVectTwo(clauseVars, mainSynonymVars[i])) {
			bool skip = true;
			for (int j = 0; j < clauseVars.size(); j += 1) {
				if (checkIfIsDigitForClause(clauseVars[j]) || clauseVars[j] != "'_'") {
					skip = false;
				}
			}
			if (skip) {
				continue;
			}
		}
		string source = clauseVars[0];
		string target = clauseVars[1];
		string mainSynonymType = declarationMap[mainSynonymVars[i]];
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
				if (
					mainSynonymType == "stmt" && targetTable == "parents" &&
					isValInMap(declarationMap, source) && isValInVectTwo({ "while", "if_table" }, declarationMap[source])
				) {
					string tempTargetAlias = "TABLE_" + source;
					string tempSourceAlias = tempTargetAlias;
					if (isValInMap(declarationMap, target) && declarationMap[target] == "stmt") {
						tempSourceAlias = "TABLE_" + target;
					}
					clause = appendAnd(clause);
					clause += tempSourceAlias + ".stmtNo != " + tempTargetAlias + ".parentStmtNo";
				}
				if (checkIfIsDigitForClause(source) || (isValInMap(declarationMap, source) && declarationMap[source] == "stmt")) {
					if (checkIfIsDigitForClause(source)) {
						clause = appendAnd(clause);
						string sourceColumnWithAlias = targetTableAlias + ".parentStmtNo";
						string op = " = ";
						if (targetTable == "nexts") {
							sourceColumnWithAlias = targetTableAlias + ".prevStmtNo";
							if (!direct) {
								op = " >= ";
								sourceColumnWithAlias = "CAST(" + targetTableAlias + ".prevStmtNo" + " AS INT)";
								source = removeQuotes(source);
							}
						}
						clause += sourceColumnWithAlias + op + source;
					}
					if (targetTable == "parents") {
						clause = appendAnd(clause);
						clause += targetTableAlias + ".stmtNo != " + targetTableAlias + ".parentStmtNo";
					}
				}
				if (
					targetTable == "nexts" &&
					isValInMap(declarationMap, target) &&
					isValInVectTwo({ "while", "if_table" }, declarationMap[target])
				) {
					clause = appendAnd(clause);
					string tempAlias = "TABLE_" + target;
					clause += tempAlias + ".isParent = '1'";
				}
				if (checkIfIsDigitForClause(target)) {
					clause = appendAnd(clause);
					string op = " = ";
					string targetColumnWithAlias = targetTableAlias + ".stmtNo";
					if (targetTable == "nexts" && !direct) {
						op = " <= ";
						targetColumnWithAlias = "CAST(" + targetTableAlias + ".stmtNo" + " AS INT)";
						target = removeQuotes(target);
					}
					clause += targetColumnWithAlias + op + target;
				}
				if (direct && !directAdded) {
					string columnName = ".direct";
					if (isValInMap(declarationMap, source) && isValInVectTwo({ "while", "if_table" }, declarationMap[source])) {
						string tempAlias = "TABLE_" + source;
						clause = appendAnd(clause);
						clause += tempAlias + ".direct = '1'";
					}
					clause = appendAnd(clause);
					clause += targetTableAlias + columnName + " = '1'";
					directAdded = true;
				}
				if (
					!isFirstAdded &&
					isValInMap(declarationMap, target) &&
					targetTable == "parents" &&
					(
						isValInVectTwo({ "while", "if_table" }, declarationMap[target]) ||
						(
							isValInMap(declarationMap, source) &&
							isValInVectTwo({ "while", "if_table" }, declarationMap[source]) &&
							mainSynonymType == "stmt" &&
							declarationMap[target] == "stmt"
						)
					)
				) {
					clause = appendAnd(clause);
					clause += targetTableAlias + ".isFirst = '0'";
					isFirstAdded = true;
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

string appendJoinOnClause(
	string joinClause, vector<string> mainSynonymVars,
	string source, string target, string patternRef,
	string toJoin, string tableToJoinAlias,
	string mainTable, string mainTableAlias, map<string, string> declarationMap
) {
	string tableToJoin = declarationMap[toJoin];
	joinClause += " INNER JOIN " + tableToJoin + " AS " + tableToJoinAlias;
	string tempJoinClause = "";
	if (tableToJoin == "nexts") {
		tempJoinClause += mainTableAlias + ".prevStmtNo = " + tableToJoinAlias + ".prevStmtNo";
	}
	else if (mainTable == "procedure" || tableToJoin == "procedure") {
		tempJoinClause = appendAnd(tempJoinClause);
		string sourceColumn = ".procedureName";
		string joinColumn = ".procedureName";
		if (mainTable == "calls" && isValInMap(declarationMap, target) && toJoin == target) {
			sourceColumn = ".targetProc";
		}
		tempJoinClause += mainTableAlias + sourceColumn + " = " + tableToJoinAlias + joinColumn;
	}
	else if (
		isValInVectTwo({ "read", "print", "assign", "stmt", "while", "if_table", "constant", "parents", "nexts", "calls" }, mainTable)
	) {
		string sourceColumn = ".stmtNo";
		string joinColumn = ".stmtNo";
		if (isValInVectTwo({ "parents", "nexts" }, mainTable)) {
			if (
				mainTable == "parents" &&
				(
					source == toJoin &&
					isValInMap(declarationMap, source) &&
					isValInVectTwo({ "while", "if_table", "stmt" }, declarationMap[toJoin])
				)
			) {
				sourceColumn = ".parentStmtNo";
				if (
					isValInMap(declarationMap, target) &&
					!isValInVectTwo({ "while", "if_table", "calls" }, declarationMap[target])
				) {
					joinColumn = ".parentStmtNo";
				}
			}
			else if (
				isValInMap(declarationMap, source) &&
				source == toJoin &&
				mainTable == "nexts"
			) {
				sourceColumn = ".prevStmtNo";
			}
			if (
				mainTable != "nexts" &&
				!(
					mainSynonymVars.size() != 1 &&
					isValInMap(declarationMap, source) &&
					isValInVectTwo({ "while", "if_table" }, declarationMap[source])
				) &&
				!checkIfIsDigitForClause(source) &&
				target == toJoin &&
				isValInMap(declarationMap, target) &&
				isValInVectTwo({ "while", "if_table" }, declarationMap[target])
			) {
				joinColumn = ".parentStmtNo";
			}
		}
		tempJoinClause = appendAnd(tempJoinClause);
		tempJoinClause += mainTableAlias + sourceColumn + " = " + tableToJoinAlias + joinColumn;
	}
	else if (isValInVectTwo({ "read", "print", "assign", "stmt", "while", "if_table", "constant", "parents" }, tableToJoin)) {
		tempJoinClause = appendAnd(tempJoinClause);
		tempJoinClause += mainTableAlias + ".stmtNo = " + tableToJoinAlias + ".stmtNo";
	}
	else if (isValInVectTwo({ "modifies", "uses", "pattern_table" }, mainTable)) {
		string sourceColumn = ".stmtNo";
		string targetColumn = ".stmtNo";
		if (isValInMap(declarationMap, toJoin) && declarationMap[toJoin] == "variable") {
			if (mainTable == "pattern_table") {
				if (toJoin == source) {
					sourceColumn = ".source";
					targetColumn = ".name";
				}
			}
			else {
				if (mainTable == "modifies" || mainTable == "uses") {
					sourceColumn = ".target";
				}
				else {
					sourceColumn = ".source";
				}
				targetColumn = ".name";
			}
		}
		else if (mainTable == "pattern_table" && (tableToJoin == "modifies" || tableToJoin == "uses")) {
			sourceColumn = ".source";
			targetColumn = ".target";
		}
		else if (
			isValInMap(declarationMap, target) &&
			isValInVectTwo({ "while", "if_table" }, tableToJoin)
		) {
			targetColumn = ".parentStmtNo";
		}
		tempJoinClause = appendAnd(tempJoinClause);
		tempJoinClause += mainTableAlias + sourceColumn + " = " + tableToJoinAlias + targetColumn;
	}
	else if (mainTable == "variable") {
		tempJoinClause = appendAnd(tempJoinClause);
		tempJoinClause += mainTableAlias + ".name = " + tableToJoinAlias;
		if (tableToJoin == "pattern_table") {
			tempJoinClause += ".source";
		}
		else {
			tempJoinClause += ".target";
		}
	}
	if (tempJoinClause != "") {
		joinClause += " ON " + tempJoinClause;
	}
	return joinClause;
}

string getToJoinAlias(vector<string> clauseVars, string targetTableAlias) {
	string toJoinAlias = "TO_JOIN_" + targetTableAlias;
	for (int i = 0; i < clauseVars.size(); i += 1) {
		string toAdd = clauseVars[i];
		if (toAdd[0] == '\'' && toAdd[toAdd.size() - 1] == '\'') {
			toAdd = removeQuotes(toAdd);
		}
		if (toAdd == "_") {
			toAdd = "wildcard";
		}
		toJoinAlias += "_" + toAdd;
	}
	return "'" + toJoinAlias + "'";
}

bool deepEqualVect(vector<string> a, vector<string> b) {
	if (a.size() != b.size()) {
		return false;
	}
	for (int i = 0; i < a.size(); i += 1) {
		if (a[i] != b[i]) {
			return false;
		}
	}
	return true;
}

string appendEndOfNestedJoin(string joinClause, vector<string> clauseVars, vector<string> mainSynonymTypes, vector<string> mainSynonymVars, vector<int> mainRefIndex, int currMainRefIdx, vector<pair<string, vector<string>>> typeToArgList, map<string, string> declarationMap, string targetTableAlias) {
	string tempJoinClause = "";
	string toJoinAlias = getToJoinAlias(clauseVars, targetTableAlias);
	for (int i = 0; i < mainSynonymVars.size(); i += 1) {
		string mainSynonymVar = mainSynonymVars[i];
		for (int j = 0; j < clauseVars.size(); j += 1) {
			if (mainSynonymVar != clauseVars[j]) {
				vector<pair<int, vector<string>>> relatedClauses;
				for (int k = currMainRefIdx; k >= 0; k -= 1) {
					vector<string> vars;
					if (k == currMainRefIdx) {
						continue;
					}
					if (
						isValInMap(declarationMap, clauseVars[j]) &&
						!isValInVectTwo(vars, clauseVars[j]) &&
						isValInVectTwo(typeToArgList[k].second, clauseVars[j])
					) {
						vars.push_back(clauseVars[j]);
					}
					if (vars.size() != 0) {
						relatedClauses.push_back({ k, vars });
					}
				}
				for (int z = 0; z < relatedClauses.size(); z += 1) {
					pair<int, vector<string>> relatedClause = relatedClauses[z];
					vector<string> relatedVars = relatedClause.second;
					string tempTableAlias = "";
					for (int k = 0; k < mainRefIndex.size(); k += 1) {
						if (
							mainRefIndex[k] < currMainRefIdx &&
							typeToArgList[mainRefIndex[k]].first == typeToArgList[relatedClause.first].first &&
							deepEqualVect(typeToArgList[mainRefIndex[k]].second, typeToArgList[relatedClause.first].second)
						) {
							tempTableAlias = getToJoinAlias(typeToArgList[relatedClause.first].second, "T_" + to_string(k + 1));
							break;
						}
					}
					if (tempTableAlias != "") {
						for (int x = 0; x < relatedVars.size(); x += 1) {
							tempJoinClause = appendAnd(tempJoinClause);
							tempJoinClause += tempTableAlias + "." + relatedVars[x] + " = " + toJoinAlias + "." + relatedVars[x];
						}
					}
				}
				continue;
			}
			tempJoinClause = appendAnd(tempJoinClause);
			string tableName = declarationMap[mainSynonymVar];
			string tableAlias = tableName + "_" + mainSynonymVar;
			tempJoinClause += tableAlias;
			if (tableName == "variable") {
				tempJoinClause += ".name = " + toJoinAlias + "." + mainSynonymVar;
			}
			else if (tableName == "procedure") {
				tempJoinClause += ".procedureName = " + toJoinAlias + "." + mainSynonymVar;
			}
			else {
				tempJoinClause += ".stmtNo = " + toJoinAlias + "." + mainSynonymVar;
			}
		}
	}
	if (tempJoinClause == "") {
		vector<pair<int, vector<string>>> relatedClauses;
		for (int i = 0; i < typeToArgList.size(); i += 1) {
			vector<string> vars;
			if (i == currMainRefIdx) {
				continue;
			}
			for (int j = 0; j < clauseVars.size(); j += 1) {
				if (isValInMap(declarationMap, clauseVars[j]) && !isValInVectTwo(vars, clauseVars[j]) && isValInVectTwo(typeToArgList[i].second, clauseVars[j])) {
					vars.push_back(clauseVars[j]);
				}
			}
			if (vars.size() != 0) {
				relatedClauses.push_back({ i, vars });
			}
		}
		for (int i = 0; i < relatedClauses.size(); i += 1) {
			pair<int, vector<string>> relatedClause = relatedClauses[i];
			vector<string> relatedVars = relatedClause.second;
			string tempTableAlias = "";
			for (int k = 0; k < mainRefIndex.size(); k += 1) {
				if (
					typeToArgList[mainRefIndex[k]].first == typeToArgList[relatedClause.first].first &&
					deepEqualVect(typeToArgList[mainRefIndex[k]].second, typeToArgList[relatedClause.first].second)
				) {

					tempTableAlias = getToJoinAlias(typeToArgList[relatedClause.first].second, "T_" + to_string(k + 1));
					break;
				}
			}
			if (tempTableAlias != "") {
				for (int j = 0; j < relatedVars.size(); j += 1) {
					tempJoinClause = appendAnd(tempJoinClause);
					tempJoinClause += tempTableAlias + "." + relatedVars[j] + " = " + toJoinAlias + "." + relatedVars[j];
				}
			}
		}
	}
	joinClause += ") as " + toJoinAlias + " ON " + tempJoinClause;

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
	int bracketCounter = 0;
	for (size_t i = 0; i < tokens.size(); i += 1) {
		string currToken = tokens.at(i);
		if (isInCondition) {
			if (currToken == "(") {
				bracketCounter += 1;
			}
			else if (currToken == ")") {
				bracketCounter -= 1;
				if (bracketCounter == 0) {
					isInCondition = false;
				}
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
				else if (isdigit(source[0]) || source == "_") {
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
	// track all the main synonym var being used (needed for multiple synonyms)
	mainRefSynonym.insert(mainRefSynonym.end(),mainSynonymVars.begin(), mainSynonymVars.end());
	// map all related clauses
	bool loopAgain = true;
	// loop through all the clauses to add all related clauses to mainRefIndex
	// loopAgain retriggers if at least 1 clause is added, so that all related synonym to the added clause is added as well
	while (loopAgain) {
		loopAgain = false;
		for (int j = 0; j < typeToArgList.size(); j += 1) {
			// check if current index is already added
			if (!isValInVectTwo(mappedIndex, to_string(j))) {
				// if any synonym matches any of the main synonym
				if (isVectinVect(typeToArgList[j].second, mainRefSynonym)) {
					mainRefIndex.push_back(j);
					mappedIndex.push_back(to_string(j));
					mainRefSynonym = addAllSynonym(mainRefSynonym, typeToArgList[j].second, declarationMap);
					loopAgain = true;
				}
			}
		}
	}

	for (int i = 0; i < mainRefIndex.size(); i += 1) {
		// loops through all the relevant clauses
		string targetTable = formatTableName(typeToArgList[mainRefIndex[i]].first); // table for this clause
		string unformattedTargetTable = typeToArgList[mainRefIndex[i]].first; // unformatted table name to check if parent or parent*
		vector<string> clauseVars = typeToArgList[mainRefIndex[i]].second;
		string source = clauseVars[0]; // LHS variable of this clause
		string target = clauseVars[1]; // RHS variable of this clause
		string patternRef = "";
		string refWhereClause = "";
		string targetTableAlias = "t_" + to_string(i + 1); // the alias for table belonging the clause
		string refSourceAlias = "TABLE_" + source; // the alias for table belonging to LHS of the clause
		string refTargetAlias = "TABLE_" + target; // the alias for table belonging to RHS of the clause
		if (clauseVars.size() > 2) {
			patternRef = clauseVars[2];
		}
		// join the clause
		joinClause += " INNER JOIN (SELECT " + getSubColumnClause(clauseVars, mainSynonymVars, declarationMap);
		joinClause += " FROM " + targetTable + " AS " + targetTableAlias;
		// join pattern ref if exists
		if (patternRef != "") {
			joinClause = appendJoinOnClause(
				joinClause, mainSynonymVars,
				source, target,
				patternRef, patternRef, "TABLE_" + patternRef,
				targetTable, targetTableAlias, declarationMap
			);
		}
		// join LHS of target table
		if (isValInMap(declarationMap, source)) {
			joinClause = appendJoinOnClause(
				joinClause, mainSynonymVars,
				source, target,
				patternRef, source, refSourceAlias,
				targetTable, targetTableAlias, declarationMap
			);
		}
		// join RHS of target table
		if (isValInMap(declarationMap, target)) {
			joinClause = appendJoinOnClause(
				joinClause, mainSynonymVars,
				source, target,
				patternRef, target, refTargetAlias,
				targetTable, targetTableAlias, declarationMap);
		}
		bool targetDirect = isDirect(unformattedTargetTable, source, target, declarationMap);
		// Append where clause
		refWhereClause = appendWhereClause(
			refWhereClause,
			targetTable, targetTableAlias,
			mainSynonymVars, clauseVars, declarationMap, targetDirect
		);
		if (
			isValInMap(declarationMap, source) &&
			isValInMap(declarationMap, target) &&
			isValInVectTwo({ "while", "if_table" }, declarationMap[source])
		) {
			if (
				isValInMap(declarationMap, target) &&
				!isValInVectTwo({ "assign", "while", "if_table", "calls", "print", "read", "variable" }, declarationMap[target])
			) {
				refWhereClause = appendAnd(refWhereClause);
				refWhereClause += refSourceAlias + ".stmtNo" + " = " + refTargetAlias + ".stmtNo";
			}
			else if (
				isValInMap(declarationMap, target) &&
				!isValInVectTwo({ "while", "if_table" }, declarationMap[target])
			) {
				refWhereClause = appendAnd(refWhereClause);
				refWhereClause += refSourceAlias + ".parentStmtNo = " + refSourceAlias + ".stmtNo";
			}
			if (isValInVectTwo({ "while", "if_table" }, declarationMap[target])) {
				if (targetTable != "nexts") {
					refWhereClause = appendAnd(refWhereClause);
					refWhereClause += "CAST(" + refSourceAlias + ".parentStmtNo as INT) < CAST(" + refTargetAlias + ".stmtNo as INT)";
				}
				refWhereClause = appendAnd(refWhereClause);
				refWhereClause += refSourceAlias + ".stmtNo != " + refTargetAlias + ".stmtNo";
			}
		}
		refWhereClause = checkAndAddDirect(refWhereClause, targetTableAlias, i, mainRefIndex, typeToArgList, declarationMap);
		if (refWhereClause != "") {
			joinClause += " WHERE " + refWhereClause;
		}
		// close off nested join
		joinClause = appendEndOfNestedJoin(joinClause, clauseVars, mainSynonymTypes, mainSynonymVars, mainRefIndex, mainRefIndex[i], typeToArgList, declarationMap, targetTableAlias);
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
		vector<string> clauseVars = typeToArgList[i].second;
		string source = clauseVars[0];
		string target = clauseVars[1];
		string irrWhereClause = "";
		bool targetDirect = isDirect(unformattedTargetTable, source, target, declarationMap);
		irrelevantClause = appendAnd(irrelevantClause);
		irrelevantClause += "(SELECT COUNT(*) FROM " + targetTable;
		irrWhereClause = appendWhereClause(
			irrWhereClause,
			targetTable, targetTable,
			mainSynonymVars, clauseVars, declarationMap, targetDirect
		);
		// irrelevantClause = checkAndAddDirect(irrelevantClause, targetTable, i, mainRefIndex, typeToArgList, declarationMap);
		if (irrWhereClause != "") {
			irrelevantClause += " WHERE " + irrWhereClause;
		}
		irrelevantClause += ") > 0";
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