#include "SourceProcessor.h"
#include <iostream>
#include <vector>

// method to check if value is found in the vector
bool isValInVect(vector<string> vector, string value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

// method to insert variable and constants from expr
void insertExpr(vector<string> loopCondition, vector<string> tokens, int currIdx, int initialOffset, int stmtNum, string procedureName) {
	int offset = initialOffset;
	string offsetToken = tokens.at(currIdx + offset);
	string concatStr;
	while (!isValInVect(loopCondition, offsetToken)) {
		concatStr += offsetToken;
		// check for values that does not match
		if (!isValInVect({ "(", ")", ">", "<", "+", "-", "*", "/", "%" }, offsetToken)) {
			if (isalpha(offsetToken[0])) {
				Database::insertVariable(offsetToken, to_string(stmtNum));
				if (initialOffset == 2) {
					// if it is an assignment
					Database::insertUses(to_string(stmtNum), procedureName, offsetToken);
				}
			}
			else if (isdigit(offsetToken[0])) {
				Database::insertConstant(offsetToken, to_string(stmtNum));
			}
		}
		offset += 1;
		offsetToken = tokens.at(currIdx + offset);
	}
	Database::insertPattern(to_string(stmtNum), tokens.at(currIdx), concatStr);
}

void insertForAllContainer(vector<pair<string, int>> containerList, int stmtNum) {
	for (int i = containerList.size() - 1; i > 0; i -= 1) {
		string direct = "1";
		if (i != containerList.size() - 1) {
			direct = "0";
		}
		Database::insertParent(to_string(stmtNum), to_string(containerList[i].second), direct);
	}
}

void insertForSpecificContainer(vector<pair<string, int>> containerList, int stmtNum, string type) {
	for (int i = containerList.size() - 1; i > 0; i -= 1) {
		string direct = "1";
		if (i != containerList.size() - 1) {
			direct = "0";
		}
		// insert nested while / if statements
		if (type == "while") {
			Database::insertWhile(to_string(stmtNum), "0", to_string(containerList[i].second), direct);
		}

		if (type == "if") {
			Database::insertIf(to_string(stmtNum), "0", to_string(containerList[i].second), direct);
		}
	}
}

// method for processing the source program
// This method currently only inserts the procedure name into the database
// using some highly simplified logic.
// You should modify this method to complete the logic for handling all the required syntax.
void SourceProcessor::process(string program) {
	// initialize the database
	Database::initialize();

	// tokenize the program
	Tokenizer tk;
	vector<string> tokens;
	tk.tokenize(program, tokens);

	// This logic is highly simplified based on iteration 1 requirements and 
	// the assumption that the programs are valid.
	string procedureName = tokens.at(1);
	// insert the procedure into the database
	Database::insertProcedure(procedureName);

	//@@@ init @@@//
	int stmtNum = 0; //statement increment
	bool isInExpr = false;
	
	vector<pair<string, int>> containerList;
	vector<pair<string, int>> whileList;
	vector<pair<string, int>> ifelseList;
	containerList.push_back({ "main", 1 });
	whileList.push_back({ "main", 1 });
	ifelseList.push_back({ "main", 1 });

	vector<string> containers;
	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {

		string prevToken = tokens.at(i - 1);
		string currToken = tokens.at(i);
		if (containers.size() > 0 && currToken == "}") {
			if (containers[containers.size() - 1] != "if") {
				if (containers[containers.size() - 1] == "while") {
					whileList.pop_back();
				}
				else {
					ifelseList.pop_back();
				}
				containers.pop_back();
				containerList.pop_back();
			}
			else {
				containers[containers.size() - 1] = "ifelse";
			}
		}
		if (isValInVect({"{", ";", "}"}, prevToken) &&
			!isValInVect({"}", "else"}, currToken)
		) {
			stmtNum++;
			Database::insertStmt(to_string(stmtNum));
			insertForAllContainer(containerList, stmtNum);
			insertForSpecificContainer(whileList, stmtNum, "while");
			insertForSpecificContainer(ifelseList, stmtNum, "if");
		}
		if (isValInVect({ "{", "then", ";" }, currToken)) {
			isInExpr = false;
		}
		else if (isInExpr) {
			// move to next iteration till end of expr
			continue;
		}
		// ------------------------------------------------------------------------
		// Note. All additional logic should start after this line
		// ------------------------------------------------------------------------
		if (isValInVect({"while", "if"}, currToken)) {
			if (currToken == "while") {
				Database::insertWhile(to_string(stmtNum), "1", to_string(stmtNum), "1");
				whileList.push_back({ currToken, stmtNum });
			}
			else if (currToken == "if") {
				Database::insertIf(to_string(stmtNum), "1", to_string(stmtNum), "1");
				ifelseList.push_back({ currToken, stmtNum });
			}
			insertForAllContainer(containerList, stmtNum);
			containers.push_back(currToken);
			containerList.push_back({ currToken, stmtNum });
			Database::insertParent(to_string(stmtNum + 1), to_string(stmtNum), "1");
			isInExpr = true;
			insertExpr({ "{", "then", ";" }, tokens, i, 1, stmtNum, procedureName);
		}
		// ensure not out of bounds
		else if (currToken != "}") {
			if (isalpha(currToken[0]) && tokens.at(i + 1) == "=") {
				isInExpr = true;
				Database::insertVariable(currToken, to_string(stmtNum));
				Database::insertAssignment(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				// offset two to skip equal sign
				insertExpr({ ";" }, tokens, i, 2, stmtNum, procedureName);
			}
			else if (prevToken == "read") {
				Database::insertVariable(currToken, to_string(stmtNum));
				Database::insertRead(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
			}
			else if (prevToken == "print") {
				Database::insertVariable(currToken, to_string(stmtNum));
				Database::insertPrint(to_string(stmtNum));
				Database::insertUses(to_string(stmtNum), procedureName, currToken);
			}
			else if (prevToken == "call") {
				// not needed for iteration 2
				//Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				//Database::insertUses(to_string(stmtNum), procedureName, currToken);
			}
		}
	}
}		