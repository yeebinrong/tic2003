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
				Database::insertVariable(offsetToken);
				if (initialOffset == 2) {
					// if it is an assignment
					Database::insertUses(to_string(stmtNum), procedureName, offsetToken);
				}
			}
			else if (isdigit(offsetToken[0])) {
				Database::insertConstant(offsetToken);
			}
		}
		offset += 1;
		offsetToken = tokens.at(currIdx + offset);
	}
	Database::insertPattern(to_string(stmtNum), tokens.at(currIdx), concatStr);
}

// method to flood insert Next*
void indirectNext(int topOfContainer, int stmtNum, vector<pair<string,int>> container) {
		for (int x = stmtNum-1; x >= topOfContainer; x--) {
			Database::insertNext(to_string(x), to_string(stmtNum), "0");
		}
		if (container.back().first == "else") {
			container.pop_back();
			Database::insertNext(to_string(container.back().second), to_string(stmtNum), "0");
		}
}

void indirectParent(int containerHead, int stmtNum) {
	for (int i = stmtNum; i > containerHead; i--)
		Database::insertParent(to_string(containerHead), to_string(i), "0");
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
	int prevStmtNum = 0; //statement-1 + skip 0->1
	//vector<int> lastIfNum;
	//int containerStmtNum = 0; //reference statement number for container (if/while)
	bool skip = false; //skip next-stmt btween if and else
	string prevState = "main";
	string curState = "main"; //main, if, else, while
	bool isInExpr = false;
	
	vector<pair<string, int>> containerList;
	containerList.emplace_back("main", 1);
	
	vector<string> containers;
	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {

		string prevToken = tokens.at(i - 1);
		string currToken = tokens.at(i);
		if (containers.size() > 0 && currToken == "}") {
			containers.pop_back();
		}
		if (isValInVect({"{", ";", "}"}, prevToken) &&
			!isValInVect({"}", "else"}, currToken)
		) {
			prevStmtNum = stmtNum;
			stmtNum++;
			Database::insertStmt(to_string(stmtNum));

			//Update container state (if/while/main)
			if (isValInVect(containers, "while")) {
				Database::insertWhile(to_string(stmtNum));
			}

			if (isValInVect(containers, "if")) {
				Database::insertIf(to_string(stmtNum));
			}
			///////////////////////////////////////////////////////////////////////////
			if (isValInVect({ "if" , "while" }, currToken)) {

				curState = currToken;
				containerList.emplace_back(curState, stmtNum);
				Database::insertParent(to_string(containerList.back().second), to_string(stmtNum + 1), "1");
			}
			//Handle Next & Parent Insert
			cout << "this is curState:" << curState << prevStmtNum << endl;
			if (prevStmtNum && !skip) {
				Database::insertNext(to_string(prevStmtNum), to_string(stmtNum), "1");

				if (curState == "else") {
					indirectNext(containerList.back().second, stmtNum, containerList); //888
				}
				else {
					//Handle while-loop self-next
					if (curState == "while") {
						Database::insertNext(to_string(stmtNum), to_string(stmtNum), "0"); //
					}
					else { //curState == "if" or "main"
						indirectNext(containerList.back().second, stmtNum, containerList); //888
					}
				}

				if (curState != "main") {
					if (containerList.back().second != stmtNum) {
						//Database::insertParent(containerList.back().first, to_string(stmtNum), "1");
						Database::insertParent(to_string(containerList.back().second), to_string(stmtNum), "1");
					}
				}
			}
			else {
				skip = false;
			}
		}
		else if (currToken == "}") { //handle end of container, update container state
			if (curState == "while") {
				prevState = curState;
				Database::insertNext(to_string(stmtNum), to_string(containerList.back().second), "1"); //while loop
				//for (int i = stmtNum; i >= containerList.back().second; i--) {
				//	indirectNext(stmtNum, i, containerList); //working
				//}
				indirectParent(containerList.back().second, stmtNum);
				containerList.pop_back();
				curState = containerList.back().first;
			}
			else if (curState == "if") {
				skip = true;
				Database::insertNext(to_string(containerList.back().second), to_string(stmtNum+1), "1");
				//tabase::insertParent(containerList.back().first, to_string(stmtNum + 1), "1");
				Database::insertParent(to_string(containerList.back().second), to_string(stmtNum+1), "1");
				prevState = curState;
				curState = "else";
				int prevContHead = containerList.back().second;
				containerList.pop_back();
				containerList.emplace_back(curState, prevContHead);
				//lastIfNum.push_back(stmtNum);
				
			}
			else if (curState == "else") {
				prevState = curState;
				//containerList.pop_back();
				//insert next if -> out of container
				indirectParent(containerList.back().second, stmtNum);
				//int prevContHead = containerList.back().second;
				containerList.pop_back();
				curState = containerList.back().first;
			}
		}
////////////////////////////////////////////////////////////////////////////
		if (isValInVect({ "{", "then", ";" }, currToken)) {
			isInExpr = false;
		}
		else if (isInExpr) {
			// move to next iteration till end of expr
			continue;
		}
		if (isValInVect({"while", "if"}, currToken)) {
			if (currToken == "while") {
				Database::insertWhile(to_string(stmtNum));
				containers.push_back("while");
			}
			else if (currToken == "if") {
				Database::insertIf(to_string(stmtNum));
				containers.push_back("if");
			}
			isInExpr = true;
			insertExpr({ "{", "then", ";" }, tokens, i, 1, stmtNum, procedureName);
		}
		// ensure not out of bounds
		else if (currToken != "}") {
			if (isalpha(currToken[0]) && tokens.at(i + 1) == "=") {
				isInExpr = true;
				Database::insertVariable(currToken);
				Database::insertAssignment(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				// offset two to skip equal sign
				insertExpr({ ";" }, tokens, i, 2, stmtNum, procedureName);
			}
			else if (prevToken == "read") {
				Database::insertVariable(currToken);
				Database::insertRead(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
			}
			else if (prevToken == "print") {
				Database::insertVariable(currToken);
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