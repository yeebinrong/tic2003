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
	str = removeWhiteSpace(str);
	string temp = "";
	for (int i = 0; i < str.size(); i += 1) {
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
	return temp;
}

string checkAndAddDirect(string whereClause, string targetTableAlias, int i, vector<int> mainRefIndex, vector<pair<string, vector<string>>> typeToArgList, map<string, string> declarationMap) {
	if (
		typeToArgList[mainRefIndex[i]].first == "Parent*" &&
		(
			typeToArgList[mainRefIndex[i]].second[0] != "_" &&
			!isValInMap(declarationMap, typeToArgList[mainRefIndex[i]].second[0])
		) &&
		(
			!isValInMap(declarationMap, typeToArgList[mainRefIndex[i]].second[1]) ||
			(
				// if RHS of clause is a synonym that is not while or if
				isValInMap(declarationMap, typeToArgList[mainRefIndex[i]].second[1]) &&
				!isValInVectTwo({ "while", "if_table" }, declarationMap[typeToArgList[mainRefIndex[i]].second[1]])
				)
			)
		) {
		whereClause = appendAnd(whereClause);
		whereClause += targetTableAlias + ".direct = '0'";
	}
	return whereClause;
}

string appendPatternClause(string clause, string tableName, string column, string value) {
	if (value != "_") {
		clause = appendAnd(clause);
		if (isExactMatch(value)) {
			clause += tableName + "." + column + " = " + removeWhiteSpace(value);
		}
		else {
			value = checkAndReplaceLike(value);
			clause += tableName + "." + column + " LIKE '" + value + "'";
		}
	}
	return clause;
}

string appendMainClause(string clause, string mainSynonymType) {
	if (mainSynonymType == "procedure") {
		clause += "procedure.procedureName FROM procedure";
	}
	else if (mainSynonymType == "variable") {
		clause += "variable.name FROM variable";
	}
	else if (mainSynonymType == "constant") {
		clause += "constant.value FROM constant";
	}
	else if (isValInVectTwo({ "assign", "print", "read", "stmt", "while", "if_table" }, mainSynonymType)) {
		clause += mainSynonymType + ".stmtNo FROM " + mainSynonymType;
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

bool isNotWhileOrIf(map<string, string> declarationMap, string value) {
	return isValInMap(declarationMap, value) && !isValInVectTwo({ "while", "if_table" }, declarationMap[value]);
}

string appendWhereClause(string clause, string targetTable, string targetTableAlias, string mainSynonymType, string source, string target, map<string, string> declarationMap, bool direct) {
	if (isValInVectTwo({ "constant", "stmt", "read", "print", "assign", "while", "if_table", "variable", "procedure" }, mainSynonymType)) {
		if (isdigit(source[0]) && targetTable != "parents") {
			clause = appendAnd(clause);
			clause += targetTableAlias + ".stmtNo = " + "'" + source + "'";
		}
		else {
			if (targetTable == "pattern_table") {
				if (!isValInMap(declarationMap, source)) {
					clause = appendPatternClause(clause, targetTableAlias, "source", source);
				}
				clause = appendPatternClause(clause, targetTableAlias, "target", target);
			}
			else {
				if (!isValInMap(declarationMap, source) && mainSynonymType == "procedure") {
					clause = appendAnd(clause);
					clause += targetTableAlias + ".procedureName = '" + source + "'";
				}
				if (isValInVectTwo({ "uses", "modifies" }, targetTable)) {
					if (isdigit(source[0])) {
						clause = appendAnd(clause);
						clause += targetTableAlias + ".stmtNo = '" + source + "'";
					}
					else if (!isValInMap(declarationMap, target) && target != "_") {
						clause = appendAnd(clause);
						clause += targetTableAlias + ".target = '" + target + "'";
					}
				}
			}
		}
		if (targetTable == "parents") {
			if (isdigit(source[0])) {
				clause = appendAnd(clause);
				clause += targetTableAlias + ".parentStmtNo = '" + source + "'";
				clause = appendAnd(clause);
				clause += targetTableAlias + ".stmtNo != " + targetTableAlias + ".parentStmtNo";
			}
			if (isdigit(target[0])) {
				clause = appendAnd(clause);
				clause += targetTableAlias + ".stmtNo = '" + target + "'";
			}
			if (direct) {
				clause = appendAnd(clause);
				clause += targetTableAlias + ".direct = '1'";
			}
			else if (isValInMap(declarationMap, target) && isValInVectTwo({ "while", "if_table" }, declarationMap[target])) {
				clause = appendAnd(clause);
				clause += targetTableAlias + ".isFirst = '0'";
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
	return value;
}

string appendJoinOnClause(string joinClause, string mainSynonymType, string source, string target, string patternRef, string targetTable, string targetTableAlias, string sourceTable, string sourceTableAlias, string mainSource, string mainTarget, map<string, string> declarationMap) {
	joinClause += " INNER JOIN " + targetTable + " AS " + targetTableAlias;
	if (targetTable == "procedure" || sourceTable == "procedure") {
		joinClause += " ON " + sourceTableAlias + ".procedureName = " + targetTableAlias + ".procedureName";
	}
	else if (
		isValInVectTwo({ "read", "print", "assign", "stmt", "while", "if_table", "constant", "parents" }, sourceTable)
	) {
		string sourceColumn = ".stmtNo";
		string joinColumn = ".stmtNo";
		if (sourceTable == "parents") {
			if (isValInMap(declarationMap, target) && isValInVectTwo({ "stmt" }, declarationMap[target])) {
				sourceColumn = ".parentStmtNo";
			}
			if (isValInMap(declarationMap, target) && mainSynonymType != declarationMap[target] && isValInVectTwo({ "while", "if_table" }, declarationMap[target])) {
				joinColumn = ".parentStmtNo";
			}
		}
		joinClause += " ON " + sourceTableAlias + sourceColumn + " = " + targetTableAlias + joinColumn;
	}
	else if (isValInVectTwo({ "read", "print", "assign", "stmt", "while", "if_table", "constant", "parents" }, targetTable)) {
		joinClause += " ON " + sourceTableAlias + ".stmtNo = " + targetTableAlias + ".stmtNo";
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
		joinClause += " ON " + sourceTableAlias + sourceColumn + " = " + targetTableAlias + targetColumn;
	}
	else if (sourceTable == "variable") {
		joinClause += " ON " + sourceTableAlias + ".name = " + targetTableAlias;
		if (targetTable == "pattern_table") {
			joinClause += ".source";
		}
		else {
			joinClause += ".target";
		}
	}
	return joinClause;
}

string appendSourceAndTargetJoin(string joinClause, string mainSynonymType, string source, string sourceTableAlias, string target, string targetTableAlias, string patternRef, string mainTable, string mainTableAlias, string mainSource, string mainTarget, map<string, string> declarationMap) {
	// join LHS of target table arg if not already joined
	if (isValInMap(declarationMap, source) && !isValInVectTwo({ mainSynonymType, mainTable }, declarationMap[source])) {
		joinClause = appendJoinOnClause(joinClause, mainSynonymType, source, target, patternRef, declarationMap[source], sourceTableAlias, mainTable, mainTableAlias, mainSource, mainTarget, declarationMap);
	}
	// join RHS of target table arg if not already joined
	if (isValInMap(declarationMap, target) && !isValInVectTwo({ mainSynonymType, mainTable, declarationMap[source] }, declarationMap[target])) {
		joinClause = appendJoinOnClause(joinClause, mainSynonymType, source, target, patternRef, declarationMap[target], targetTableAlias, mainTable, mainTableAlias, mainSource, mainTarget, declarationMap);
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
	// init synonym as tokens.at(0) as it is possible the expected column is not defined as a variable based on example
	string mainSynonymType = formatTableName(tokens.at(0));
	string mainSynonymVar = tokens.at(0);
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
				int offset = 2;
				string source = tokens.at(i + offset);
				if (source == "\"") {
					offset += 1;
					source = tokens.at(i + offset);
					offset += 1;
				}
				offset += 3;
				string target = tokens.at(i + offset);
				if (tokens.at(i + offset - 1)[0] != '"') {
					target = tokens.at(i + offset - 1);
				}
				typeToArgList.push_back({ currToken, { source, target } });
			}
			else if (currToken == "pattern") {
				isInCondition = true;
				int offset = i + 3;
				string patternRef = tokens.at(i + 1);
				// source can be _ (match all) or "variable name" (exact match) or synonymTypeVar
				string source = checkExactMatch(offset, tokens);
				if (source[0] != '_' && tokens.at(i + 3) == "\"") {
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
		else if (currToken == ";" || currToken == ",") {
			string synonymType = formatTableName(tokens.at(i - 2));
			if (synonymType == ",") {
				synonymType = declarationMap[tokens.at(i - 3)];
			}
			// new declaration, insert as { name : { synonymType : synonymTypeAlias } }
			declarationMap.insert({ tokens.at(i - 1), synonymType });
		}
		else if (currToken == "Select") {
			// Map the expected column if exists, else use default
			if (isValInMap(declarationMap, tokens.at(i + 1))) {
				mainSynonymType = formatTableName(declarationMap.at(tokens.at(i + 1)));
				mainSynonymVar = tokens.at(i + 1);
			}
			joinedTables.push_back(mainSynonymType);
			// End of declaration, start parsing query
			isEndOfDeclaration = true;
		}
	}

	string joinClause = "";
	string whereClause = "";
	vector<string> mappedIndex; // track for typeToArgMap already used
	vector<int> mainRefIndex; // track index of clauses that are related to main synonym
	vector<string> mainRefSynonym; // track synonyms of clauses that are related to main synonym, directly or indirectly
	mainRefSynonym.push_back(mainSynonymVar);
	// map all related clauses
	bool loopAgain = true;
	bool hasIfOrWhileOrStmtInParent = false;
	while (loopAgain) {
		loopAgain = false;
		for (int j = 0; j < typeToArgList.size(); j += 1) {
			if (!isValInVectTwo(mappedIndex, to_string(j))) {
				if (isVectinVect(typeToArgList[j].second, mainRefSynonym)) {
					if (
						formatTableName(typeToArgList[j].first) == "parents" &&
						(
							(isValInMap(declarationMap, typeToArgList[j].second[0]) && isValInVectTwo({ "while", "if_table", "stmt" }, declarationMap[typeToArgList[j].second[0]])) ||
							(isValInMap(declarationMap, typeToArgList[j].second[1]) && isValInVectTwo({ "while", "if_table", "stmt" }, declarationMap[typeToArgList[j].second[1]]))
						)
					) {
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
		string target = typeToArgList[mainRefIndex[0]].second[1];
		string patternRef = "";
		if (typeToArgList[mainRefIndex[0]].second.size() > 2) {
			patternRef = typeToArgList[mainRefIndex[0]].second[2];
		}
		// only 1 relevant clauses, join target table normally
		// eg. Select a such that Uses (a, v)

		joinClause = appendJoinOnClause(joinClause, mainSynonymType, source, target, patternRef, targetTable, targetTable, mainSynonymType, mainSynonymType, source, target, declarationMap);
		joinClause = appendSourceAndTargetJoin(
			joinClause,
			mainSynonymType,
			source, isValInMap(declarationMap, source) ? declarationMap[source] : "",
			target, isValInMap(declarationMap, target) ? declarationMap[target] : "",
			patternRef, mainSynonymType, mainSynonymType,
			source, target, declarationMap
		);
		bool direct = isDirect(unformattedTargetTable);
		// append where clause
		whereClause = appendWhereClause(whereClause, targetTable, targetTable, mainSynonymType, source, target, declarationMap, direct);
		whereClause = checkAndAddDirect(whereClause, targetTable, 0, mainRefIndex, typeToArgList, declarationMap);
	}
	// ------------------------------------- AT LEAST 1 RELEVANT CLAUSE, NEST JOINS -------------------------------------
	else if (mainRefIndex.size() > 0) {
		// more than 1 relevant clauses and has parent, nest joins
		string mainTable = formatTableName(typeToArgList[mainRefIndex[0]].first);
		string mainSource = typeToArgList[mainRefIndex[0]].second[0];
		string mainTarget = typeToArgList[mainRefIndex[0]].second[1];
		string mainTableAlias = "t_1"; // the table belonging the clause
		string refWhereClause = "";
		for (int i = 0; i < mainRefIndex.size(); i += 1) {
			string targetTable = formatTableName(typeToArgList[mainRefIndex[i]].first);
			string unformattedTargetTable = typeToArgList[mainRefIndex[i]].first;
			string source = typeToArgList[mainRefIndex[i]].second[0];
			string target = typeToArgList[mainRefIndex[i]].second[1];
			string patternRef = "";
			if (typeToArgList[mainRefIndex[i]].second.size() > 2) {
				patternRef = typeToArgList[mainRefIndex[i]].second[2];
			}
			string targetTableAlias = "t_" + to_string(i + 1); // the table belonging the clause
			string refSourceAlias = "t_source_" + to_string(i + 1); // the table belonging to LHS of the clause
			string refTargetAlias = "t_target_" + to_string(i + 1); // the table belonging to RHS of the clause
			if (i == 0) {
				joinClause += " INNER JOIN (SELECT * FROM " + mainTable + " AS " + mainTableAlias;
			}
			else {
				joinClause = appendJoinOnClause(joinClause, mainSynonymType, source, target, patternRef, targetTable, targetTableAlias, mainTable, mainTableAlias, mainSource, mainTarget, declarationMap);
			}
			joinClause = appendSourceAndTargetJoin(
				joinClause,
				mainSynonymType,
				source, refSourceAlias,
				target, refTargetAlias,
				patternRef, mainTable, mainTableAlias,
				mainSource, mainTarget,
				declarationMap
			);
			bool targetDirect = isDirect(unformattedTargetTable);
			refWhereClause = appendWhereClause(
				refWhereClause,
				targetTable, targetTableAlias,
				mainSynonymType,
				source, target,
				declarationMap, targetDirect
			);
			refWhereClause = checkAndAddDirect(refWhereClause, targetTableAlias, i, mainRefIndex, typeToArgList, declarationMap);
		}
		if (refWhereClause != "") {
			joinClause += " WHERE " + refWhereClause;
		}
		joinClause += ") as MAIN_REF_TABLE ON " + mainSynonymType;
		if (mainSynonymType == "variable") {
			joinClause += ".name = MAIN_REF_TABLE";
			if (isValInVectTwo({ "modifies", "uses" }, formatTableName(typeToArgList[mainRefIndex[0]].first))) {
				joinClause += ".target";
			}
			else {
				joinClause += ".source";
			}
		}
		else if (mainSynonymType == "procedure") {
			joinClause += ".procedureName = MAIN_REF_TABLE.procedureName";
		}
		else {
			string joinColumn = ".stmtNo";
			if (
				isValInMap(declarationMap, mainSource) &&
				isValInVectTwo({ "stmt", "while", "if_table" }, mainSynonymType) &&
				declarationMap[mainSource] == mainSynonymType
			) {
				joinColumn = ".parentStmtNo";
			}
			joinClause += ".stmtNo = MAIN_REF_TABLE" + joinColumn;
		}
	}
	string irrelevantClause = "";
	// ------------------------------------- CHECK ALL REMAINING IRRELEVANT CLAUSES -------------------------------------
	for (int i = 0; i < typeToArgList.size(); i += 1) {
		if (isValInVectTwo(mappedIndex, to_string(i))) {
			// skipped index that are already mapped
			continue;
		}
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
			mainSynonymType,
			source, target,
			declarationMap, targetDirect
		);
		//irrelevantClause = checkAndAddDirect(irrelevantClause, targetTable, i, mainRefIndex, typeToArgList, declarationMap);
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