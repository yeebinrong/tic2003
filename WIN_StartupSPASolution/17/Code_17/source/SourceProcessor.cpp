#include "SourceProcessor.h"
#include <iostream>

// method to check if value is found in the vector
bool isValInVect(vector<string> vector, string value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

// method to insert variable and constants from expr
void insertExpr(vector<string> loopCondition, vector<string> tokens, int currIdx, int initialOffset, int stmtNum, string procedureName) {
	int offset = initialOffset;
	string offsetToken = tokens.at(currIdx + offset);
	while (!isValInVect(loopCondition, offsetToken)) {
		// check for values that does not match
		if (!isValInVect({ "(", ")", ">", "<", "+", "-", "*", "/", "%" }, offsetToken)) {
			if (isalpha(offsetToken[0])) {
				Database::insertVariable(offsetToken);
				if (offset == 2) {
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
}

// method to flood insert Next*
void indirectNext(int start, int stmtNum) {
	for (int x = start; x > 0; x--) {
		Database::insertNext(to_string(x), to_string(stmtNum), "0");
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
	int prevStmtNum = 0; //statement-1 + skip 0->1
	int containerStmtNum = 0; //reference statement number for container (if/while)
	string curState = "main"; //main, if, else, while
	bool isInExpr = false;

	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {

		string prevToken = tokens.at(i - 1);
		string currToken = tokens.at(i);

		if (isValInVect({ "{", ";", "}" }, prevToken) &&
			!isValInVect({ "}", "else" }, currToken)
			) {
			prevStmtNum = stmtNum;
			stmtNum++;
			Database::insertStmt(to_string(stmtNum));

			//Update container state (if/while/main)
			if(isValInVect({ "if" , "while" }, currToken)){
				containerStmtNum = stmtNum;
				curState = currToken;
			}

			//Handle Next & Parent Insert
			if (prevStmtNum && curState != "skip") {
				Database::insertNext(to_string(prevStmtNum), to_string(stmtNum), "1");

				//Handle Next*
				if (curState == "else") {
					indirectNext(containerStmtNum, stmtNum);
				}
				else {
					indirectNext(stmtNum - 1, stmtNum);
					//Handle while-loop self-next
					if (curState == "while") {
						Database::insertNext(to_string(stmtNum), to_string(stmtNum), "0");
					}
				}

				if (curState != "main") {
					Database::insertParent(to_string(containerStmtNum), to_string(stmtNum), "1");
				}
			}
			else if (curState == "skip") { //Handle when transiting to else
				curState = "else";
				Database::insertNext(to_string(containerStmtNum), to_string(stmtNum), "1");
				indirectNext(containerStmtNum, stmtNum);
				Database::insertParent(to_string(containerStmtNum), to_string(stmtNum), "1");
			}
		}
		else if (currToken == "}") { //handle end of container, update container state
			if (curState == "while") {
				Database::insertNext(to_string(stmtNum), to_string(containerStmtNum), "1"); //while loop
				Database::insertNext(to_string(containerStmtNum), to_string(stmtNum + 1), "1"); //out of while-loop
				curState = "main";
			}
			else if (curState == "if") {
				curState = "skip";
			}
			else if (curState == "else") {
				curState = "main";
			}
		}

		if (isValInVect({ "{", "then", ";" }, currToken)) {
			isInExpr = false;
		}
		else if (isInExpr) {
			// move to next iteration till end of expr
			continue;
		}

		if (isValInVect({ "while", "if" }, currToken)) {
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
				// use calls table instead?
				//Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				//Database::insertUses(to_string(stmtNum), procedureName, currToken);
			}
		}
	}
}		