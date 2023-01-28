#include "SourceProcessor.h"
#include <iostream>

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
	// iterate subsequent statements for variable/constant
	for (int i = 2; i < tokens.size(); i++) {
		//for (string token : tokens) {
		string tempToken = tokens.at(i);
		if (tempToken != "}") {
			if ((tempToken[0] > 96) && ((tokens.at(i + 1) == "=") || (tokens.at(i - 1) == "read"))) {		// =
				Database::insertVariable(tempToken);
			}
			else if (tempToken[0] >= 48 && tempToken[0] <= 57) {	//constants
				Database::insertConstant(tempToken);
			}
		}
		
		if (((tokens.at(i - 1) == "{") || (tokens.at(i - 1) == ";" || (tokens.at(i - 1) == "}")))
				&& (tempToken != "}") && (tempToken != "else")) {
			stmtNum++;
			if (tokens.at(i + 1) == "=") {
				Database::insertAssignment(to_string(stmtNum));
				//Database::insertAssignment(tempToken);
			}
		}
		

	}

}		