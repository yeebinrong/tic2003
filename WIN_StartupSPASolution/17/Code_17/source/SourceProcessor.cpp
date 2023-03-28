#include "SourceProcessor.h"
#include <iostream>
#include <vector>
#include <map>

// method to check if value is found in the vector
bool isValInVect(vector<string> vector, string value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

// method to insert variable and constants from expr
void insertExpr(vector<string> loopCondition, vector<string> tokens, int currIdx, int initialOffset, int stmtNum, string procedureName, vector<pair<string, int>> containerList) {
	int offset = initialOffset;
	string offsetToken = tokens.at(currIdx + offset);
	string concatStr;
	while (!isValInVect(loopCondition, offsetToken)) {
		// add delimiter to separate token to prevent false positive eg. count + 1 and count + 123
		concatStr += '|' + offsetToken + '|';
		// check for values that does not match
		if (!isValInVect({ "(", ")", ">", "<", "+", "-", "*", "/", "%" }, offsetToken)) {
			if (isalpha(offsetToken[0])) {
				Database::insertVariable(procedureName, offsetToken, to_string(stmtNum));
				Database::insertUses(to_string(stmtNum), procedureName, offsetToken);
				for (int i = containerList.size() - 1; i > 0; i -= 1) {
					Database::insertUses(to_string(containerList[i].second), procedureName, offsetToken);
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

void insertForSubProc(vector<pair<string, int>> containerList, int stmtNum) {
	for (int i = containerList.size() - 1; i > 0; i -= 1) {
		string direct = "0";
		Database::insertParent(to_string(stmtNum), to_string(containerList[i].second), direct);
	}
}

void adhocProcParentPrint(string procedure, vector<pair<string, int>> containerList) {


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
	int stmtNum = 0, prevStmtNum = 0; //statement increment
	bool isInExpr = false;
	vector<pair<string, int>> containerList;
	vector<pair<string, int>> whileList;
	vector<pair<string, int>> ifelseList;
	map<string, vector<pair<string, int>>> procContMap; //store procedure and vector of containers within procedure
	vector<string> procedureList; 
	procedureList.push_back(procedureName);
	containerList.push_back({ "main", 1 });
	whileList.push_back({ "main", 1 });
	ifelseList.push_back({ "main", 1 });
	procContMap.insert(pair<string, vector<pair<string, int>>>(procedureName, {}));
	bool repeated = false;
	vector<string> containers;
	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {
		cout << "no: "<<i<<" token: " << tokens.at(i) << endl;
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
			!isValInVect({"}", "else", "procedure"}, currToken))
		{
			prevStmtNum = stmtNum;
			stmtNum++;
			Database::insertStmt(to_string(stmtNum));
			insertForAllContainer(containerList, stmtNum);
			insertForSpecificContainer(whileList, stmtNum, "while");
			insertForSpecificContainer(ifelseList, stmtNum, "if");
			//add logic here to handle sub-procedure parent* logic ************************
			if (procContMap[procedureList.back()].size()) {
				vector<pair<string, int>> tempContainerList = procContMap[procedureList.back()];
				insertForSubProc(tempContainerList, stmtNum);
			}
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
				cout << "inserting while manually! stmtno: "<<stmtNum << endl;
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
			insertExpr({ "{", "then", ";" }, tokens, i, 1, stmtNum, procedureName, containerList);
		}
		// ensure not out of bounds
		else if (currToken != "}") {
			procedureName = procedureList.back();
			if (isalpha(currToken[0]) && tokens.at(i + 1) == "=") {
				isInExpr = true;
				Database::insertVariable(procedureName, currToken, to_string(stmtNum));
				Database::insertAssignment(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				for (int i = containerList.size() - 1; i > 0; i -= 1) {
					Database::insertModifies(to_string(containerList[i].second), procedureName, currToken);
				}
				// offset two to skip equal sign
				insertExpr({ ";" }, tokens, i, 2, stmtNum, procedureName, containerList);
			}
			else if (prevToken == "read") {
				Database::insertVariable(procedureName, currToken, to_string(stmtNum));
				Database::insertRead(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				for (int i = containerList.size() - 1; i > 0; i -= 1) {
					Database::insertModifies(to_string(containerList[i].second), procedureName, currToken);
				}
			}
			else if (prevToken == "print") {
				Database::insertVariable(procedureName, currToken, to_string(stmtNum));
				Database::insertPrint(to_string(stmtNum));
				Database::insertUses(to_string(stmtNum), procedureName, currToken);
				for (int i = containerList.size() - 1; i > 0; i -= 1) {
					Database::insertUses(to_string(containerList[i].second), procedureName, currToken);
				}
			}
			else if (prevToken == "call") {
				//Database::insertModifies(to_string(stmtNum), procedureList.back(), currToken);
				//Database::insertUses(to_string(stmtNum), procedureList.back(), currToken);
				//merge and store container list onto map, for reference when handling parent* relation
				vector<pair<string, int>> mergedContainerList = containerList;
				mergedContainerList.insert(mergedContainerList.end(), procContMap[procedureList.back()].begin(), procContMap[procedureList.back()].end());
				if (procContMap.find(currToken) == procContMap.end()) {
					cout << "new entry into map" << endl;
					mergedContainerList.erase(mergedContainerList.begin());
					procContMap.insert(pair<string, vector<pair<string, int>>>(currToken, mergedContainerList));
				}
				else {
					cout << "already exists in map" << endl;
					mergedContainerList.insert(mergedContainerList.end(), procContMap[currToken].begin(), procContMap[currToken].end());
					procContMap[currToken] = mergedContainerList;
				}


			}
			else if (prevToken == "procedure") {
				cout << "this is " << currToken << "'s containerList" << endl;;
				for (int i = procContMap[currToken].size() - 1; i >= 0; i--) {
					cout << procContMap[currToken].at(i).first << ": " << procContMap[currToken].at(i).second << endl;
				}
				Database::insertProcedure(currToken);
				//procedureList.back() holds the current procedure that's being handled
				procedureList.push_back(currToken);
				//empty containerList
				containerList.clear();
			}
			
		}
		if (i+1 == tokens.size()) {
			if (repeated == false) {
				i = 2; //repeat to update parent
				repeated = true;
				stmtNum = 0;
				cout << "repeated!" << endl;
			}
			else {
				break;
			}
		}
	}
}		