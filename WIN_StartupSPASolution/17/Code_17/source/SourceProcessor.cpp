#include "SourceProcessor.h"
#include <iostream>

// method to check if value is found in the vector
bool isValInVect(vector<string> vector, string value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

// method to insert variable and constants from expr
void insertExpr(vector<string> loopCondition, vector<string> tokens, int currIdx, int initialOffset) {
	int offset = initialOffset;
	string offsetToken = tokens.at(currIdx + offset);
	while (!isValInVect(loopCondition, offsetToken)) {
		// check for values that does not match
		if (!isValInVect({ "(", ")", ">", "<", "+", "-", "*", "/", "%" }, offsetToken)) {
			if (isalpha(offsetToken[0])) {
				Database::insertVariable(offsetToken);
			}
			else if (isdigit(offsetToken[0])) {
				Database::insertConstant(offsetToken);
			}
		}
		offset += 1;
		offsetToken = tokens.at(currIdx + offset);
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
	int stmtNum = 0;
	bool isInExpr = false;
	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {
		string prevToken = tokens.at(i - 1);
		string currToken = tokens.at(i);
		cout << currToken << endl;
		if (isValInVect({"{", ";", "}"}, prevToken) &&
			!isValInVect({"}", "else"}, currToken)
		) {
			stmtNum++;
			Database::insertStmt(to_string(stmtNum));
			if (tokens.at(i + 1) == "=") {
				Database::insertAssignment(to_string(stmtNum));
			}
		}
		if (isValInVect({ "{", "then", ";"}, currToken)) {
			isInExpr = false;
		}
		else if (isInExpr) {
			// move to next loop till end of expr
			continue;
		}
		if (isValInVect({"while", "if"}, currToken)) {
			isInExpr = true;
			insertExpr({ "{", "then", ";" }, tokens, i, 1);
		}
		// ensure not out of bounds
		else if (currToken != "}") {
			if (isalpha(currToken[0]) && tokens.at(i + 1) == "=") {
				isInExpr = true;
				Database::insertVariable(currToken);
				// offset two to skip equal sign
				insertExpr({ ";" }, tokens, i, 2);
			}
			else if (prevToken == "read") {
				Database::insertVariable(currToken);
				Database::insertRead(to_string(stmtNum));
			}
			else if (prevToken == "print") {
				Database::insertVariable(currToken);
				Database::insertPrint(to_string(stmtNum));
			}
		}
	}
}