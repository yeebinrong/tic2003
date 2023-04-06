#include "SourceProcessor.h"
#include <iostream>
#include <vector>
#include <map>

// method to check if value is found in the vector
bool isValInVect(vector<string> vector, string value) {
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

//inherit use/mod relationship to container header
void insertUseModForContHeader(vector<pair<string, int>> containerList, string procedureName, string varName, string mode) {

	for (int i = containerList.size() - 1; i >= 0; i -= 1) {
		string containerHeader = containerList[i].first;
		int containerHeaderNum = containerList[i].second;
		if (containerList[i].first == "main") {
			continue;
		}
		if (mode == "use") {
			Database::insertUses(to_string(containerHeaderNum), procedureName, varName);
		}
		if (mode == "mod") {
			Database::insertModifies(to_string(containerHeaderNum), procedureName, varName);
		}
	}
}


//inherit use/mod relationship to all procedures that calls currProc
void insertForIndirectUseMod(string currProc, string varName, map<string, vector<pair<string, int>>> procCallMap, string mode) {
	vector<pair<string, int>> sourceProcList = procCallMap[currProc];
	for (int i = sourceProcList.size() - 1; i >= 0; i -= 1) {
		string sourceProc = sourceProcList[i].first;
		int sourceCallStmtNo = sourceProcList[i].second;
		if (mode == "use") {
			Database::insertUses(to_string(sourceCallStmtNo), sourceProc, varName);
		}
		if (mode == "mod") {
			Database::insertModifies(to_string(sourceCallStmtNo), sourceProc, varName);
		}
		//recursively call function to insert indirect call  for procedures that calls sourceProc
		insertForIndirectUseMod(sourceProc, varName, procCallMap, mode);
	}
}

// method to insert variable and constants from expr
void insertExpr(vector<string> loopCondition, vector<string> tokens, int currIdx, int initialOffset,
		int stmtNum, string procedureName, vector<pair<string, int>> containerList, map<string, vector<pair<string, int>>> procCallMap) {
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
				insertForIndirectUseMod(procedureName, offsetToken, procCallMap, "use");
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
	for (int i = containerList.size() - 1; i >= 0; i -= 1) {
		string direct = "1";
		if (i != containerList.size() - 1) {
			direct = "0";
		}
		if (containerList[i].first == "main") {
			continue;
		}
		Database::insertParent(to_string(stmtNum), to_string(containerList[i].second), direct, "0");
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

void insertParentForSubProc(vector<pair<string, int>> containerList, int stmtNum) {
	for (int i = containerList.size() - 1; i > 0; i -= 1) {
		string direct = "0";
		Database::insertParent(to_string(stmtNum), to_string(containerList[i].second), direct, "0");
		if (containerList[i].first == "while") {
			Database::insertWhile(to_string(stmtNum), "0", to_string(containerList[i].second), direct);
		}
		if (containerList[i].first == "if") {
			Database::insertIf(to_string(stmtNum), "0", to_string(containerList[i].second), direct);
		}
	}
}

//insert call for direct and inherit to all direct/indirect procedures that calls targetProc
void insertForAllCalls(string targetProc, int currProcCallNo, string currProc, map<string, vector<pair<string, int>>> procCallMap) {
	vector<pair<string, int>> sourceProcList = procCallMap[currProc];
	for (int i = sourceProcList.size() - 1; i >= 0; i -= 1) {
		string sourceProc = sourceProcList[i].first;
		int sourceCallStmtNo = sourceProcList[i].second;
		if (targetProc == currProc) {
			//occurs only for the 1st call of function
			Database::insertCall(sourceProc, targetProc, to_string(sourceCallStmtNo), to_string(sourceCallStmtNo), "1");
		}
		else {
			//occurs for remaining recursive calls
			Database::insertCall(sourceProc, targetProc, to_string(sourceCallStmtNo), to_string(currProcCallNo), "0");
		}
		//recursively call function to insert indirect call  for procedures that calls sourceProc
		insertForAllCalls(targetProc, sourceCallStmtNo, sourceProc, procCallMap);
	}
}

vector<vector<int>> insertIPSNL(vector<vector<int>> indPrevStmtNumList, int stmtNum) {
	if (indPrevStmtNumList.size()) {
		indPrevStmtNumList.back().push_back(stmtNum);
	}
	else {
		indPrevStmtNumList.push_back({ stmtNum });
	}
	return indPrevStmtNumList;
}



void insertContIndirectNext(int stmtNum, vector<pair<string,int>> containers) {
	while (containers.size()) { //iterate to the largest container
		int endPoint = containers.back().second;
		int startPoint = stmtNum + 1;
		int iterateFor = 1;
		if (containers.back().first == "while") {
			iterateFor = stmtNum - endPoint;
		}
		for (int x = 0; x < iterateFor; x++) {
			startPoint -= 1;
			for (int i = stmtNum; i >= endPoint; i--) {
				Database::insertNext(to_string(i), to_string(startPoint), "0");
			}
		}
		if (containers.back().first == "ifelse") {
			containers.pop_back(); 
		}
		containers.pop_back();
	}
}

void insertIndirectNext(int start, int end) {
	for (int i = start; i >= end; i--) {
		Database::insertNext(to_string(i), to_string(start),  "0");
	}
}

void insertIfElseIndirectNext(int currStmtNum, vector<int> prevList) {
	for (int i = prevList.size()-1; i > 0 ; i--) {
		Database::insertNext(to_string(prevList.at(i)), to_string(currStmtNum), "0");
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
	string prevProcedure = procedureName;
	// insert the procedure into the database
	Database::insertProcedure(procedureName);
	//@@@ init @@@//
	int stmtNum = 0, prevStmtNum = 0; //statement increment
	bool isInExpr = false;
	vector<pair<string, int>> containerList;
	vector<pair<string, int>> whileList;
	vector<pair<string, int>> ifelseList;
	map<string, vector<pair<string, int>>> procContMap; //store procedure and vector of containers within procedure
	map<string, vector<pair<string, int>>> procCallMap; //store procedure and vector of procs that calls this proc
	vector<string> procedureList; 
	int procedureStart;
	vector<pair<string, vector<int>>> containerEndList = { {"",{}} }; //store container and endStmtList pair
	map<string,vector<int>> prevStmtNumList; //list of prevStmt to relate NEXT to stmtNum <procName,list>
	vector<vector<int>> indPrevStmtNumList = {}; //store stmts that have been executed before stmtNum (for indirectNEXT only)
	procedureList.push_back(procedureName);
	procedureStart = 1;
	containerList.push_back({ "main", 1 });
	whileList.push_back({ "main", 1 });
	ifelseList.push_back({ "main", 1 });
	procContMap.insert(pair<string, vector<pair<string, int>>>(procedureName, {}));
	bool repeated = false;
	bool whileSkip = false;
	vector<pair<string,int>> containers; //if, ifelse, while (+stmtNum)
	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {
		string prevToken = tokens.at(i - 1);
		string currToken = tokens.at(i);
		if (containers.size() > 0 && currToken == "}") {
			if (containers[containers.size() - 1].first != "if") {
				if (containers[containers.size() - 1].first == "while") {//while
					vector<int> temp = containerEndList.back().second;
					if (find(temp.begin(), temp.end(), stmtNum) == temp.end()) {
						containerEndList.back().second.push_back(containerList.back().second);
						containerEndList.back().second.push_back(stmtNum);
						Database::insertNext(to_string(stmtNum), to_string(containerList.back().second),"1");
						whileSkip = true;
					}
					else {
						containerEndList.back().second.pop_back();
						Database::insertNext(to_string(containerEndList.back().second.back()), to_string(containerList.back().second), "1");
						prevStmtNumList[procedureName].push_back(containerList.back().second);
					}
					indPrevStmtNumList.pop_back();
					whileList.pop_back();
				}
				else { //ifelse
					containerEndList.back().second.push_back(stmtNum);
					prevStmtNumList[procedureName] = containerEndList.back().second; //pass consolidated end points to list, this list will be used when stmtNum increments
					indPrevStmtNumList.pop_back(); //erase temp list used for ifelse
					//merge refList + ifStmts with ifelseStmts
					for (int i = ifelseList.back().second; i <= stmtNum; i++) {
						indPrevStmtNumList = insertIPSNL(indPrevStmtNumList, i);
					}
					ifelseList.pop_back();
					containers.pop_back();//pop ifelse before pop if
				}
				containers.pop_back();
				containerList.pop_back();
			}
			else { //if
				containerEndList.push_back({ containerList.back().first, {stmtNum} });//insert to endStmtList vector for current container
				for (int i = 0; i < prevStmtNumList[procedureName].size(); i++) {
					containerEndList.back().second.push_back(prevStmtNumList[procedureName].at(i));
				}
				containers.push_back({ "ifelse", stmtNum + 1 });
				indPrevStmtNumList.push_back(indPrevStmtNumList.back()); //store list instance, move forward, next cell used for temp list (if)
				insertIfElseIndirectNext(stmtNum, indPrevStmtNumList.back());//insert indirect next
				vector<int> temp = indPrevStmtNumList.back(); //store reflist+ifStmts
				indPrevStmtNumList.pop_back(); //erase temp list used for if
				indPrevStmtNumList.push_back(indPrevStmtNumList.back()); //copy refList forward,  next cell used for temp list (ifelse)
				indPrevStmtNumList.at(indPrevStmtNumList.size() - 2) = temp; //replace refList with refList+ifStmts

			}
		}
		//STMT NUM INCREMENT//
		if (isValInVect({"{", ";", "}"}, prevToken) &&
			!isValInVect({"}", "else", "procedure"}, currToken))
		{
			prevStmtNum = stmtNum;
			if (tokens.at(i-2) == "else") {
				prevStmtNum = containerList.back().second; //if-false (else) portion
			}
			stmtNum+=1;
			Database::insertStmt(to_string(stmtNum));
			insertForAllContainer(containerList, stmtNum);
			insertForSpecificContainer(whileList, stmtNum, "while");
			insertForSpecificContainer(ifelseList, stmtNum, "if");

			//NEXT AND PARENT INSERT LOGIC PER STMT INCREMENT//
			if (!containers.size()) { //main
				insertIndirectNext(stmtNum, procedureStart);
				indPrevStmtNumList = insertIPSNL(indPrevStmtNumList, stmtNum);
			}
			else if (containers.back().first == "while") { //while
				insertContIndirectNext(stmtNum, containers);
				indPrevStmtNumList = insertIPSNL(indPrevStmtNumList, stmtNum);
			}
			else { //if + ifelse
				if (indPrevStmtNumList.size()) {
					insertIfElseIndirectNext(stmtNum, indPrevStmtNumList.back());//insert indirect next
				}
				insertContIndirectNext(stmtNum, containers);
				indPrevStmtNumList = insertIPSNL(indPrevStmtNumList, stmtNum);
			}

			//handle general NEXT relation
			if (prevStmtNum && (prevProcedure == procedureName) && !whileSkip) { //add a flag to skip end of while
				Database::insertNext(to_string(prevStmtNum), to_string(stmtNum), "1");
			}
			else {
				whileSkip = false;
				prevProcedure = procedureName;
			}
			//handle end of if-else + while => NEXT relation 
			while (prevStmtNumList[procedureName].size()) {
				prevStmtNum = prevStmtNumList[procedureName].back();
				Database::insertNext(to_string(prevStmtNum), to_string(stmtNum), "1");
				prevStmtNumList[procedureName].pop_back();
			}


			//------------------------------------//
			// logic to handle parent across proc
			//------------------------------------//
			//if (procContMap[procedureList.back()].size()) {
			//	vector<pair<string, int>> tempContainerList = procContMap[procedureList.back()];
			//	insertParentForSubProc(tempContainerList, stmtNum + 1);
			//}
			
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
				indPrevStmtNumList.push_back(indPrevStmtNumList.back()); //pass to curr list to next vector
			}
			insertForAllContainer(containerList, stmtNum);
			containers.push_back({ currToken,stmtNum });
			containerList.push_back({ currToken, stmtNum });
			string isFirst = containers.size() == 1 ? "1" : "0";
			Database::insertParent(to_string(stmtNum), to_string(stmtNum), "1", isFirst);
			Database::insertParent(to_string(stmtNum + 1), to_string(stmtNum), "1", "0");
			isInExpr = true;
			insertExpr({ "{", "then", ";" }, tokens, i, 1, stmtNum, procedureName, containerList, procCallMap);
		}
		// ensure not out of bounds
		else if (currToken != "}") {
			procedureName = procedureList.back();
			if (isalpha(currToken[0]) && tokens.at(i + 1) == "=") {
				isInExpr = true;
				Database::insertVariable(procedureName, currToken, to_string(stmtNum));
				Database::insertAssignment(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				insertForIndirectUseMod(procedureName, currToken, procCallMap, "mod");
				insertUseModForContHeader(containerList, procedureName, currToken, "mod");
				// offset two to skip equal sign
				insertExpr({ ";" }, tokens, i, 2, stmtNum, procedureName, containerList, procCallMap);
			}
			else if (prevToken == "read") {
				Database::insertVariable(procedureName, currToken, to_string(stmtNum));
				Database::insertRead(to_string(stmtNum));
				Database::insertModifies(to_string(stmtNum), procedureName, currToken);
				insertForIndirectUseMod(procedureName, currToken, procCallMap, "mod");
				insertUseModForContHeader(containerList, procedureName, currToken, "mod");
			}
			else if (prevToken == "print") {
				Database::insertVariable(procedureName, currToken, to_string(stmtNum));
				Database::insertPrint(to_string(stmtNum));
				Database::insertUses(to_string(stmtNum), procedureName, currToken);
				insertForIndirectUseMod(procedureName, currToken, procCallMap, "use");
				insertUseModForContHeader(containerList, procedureName, currToken, "use");
			}
			else if (prevToken == "call") {
				//update info with who calls procedure 'currToken'
				if (procCallMap.find(currToken) == procCallMap.end()) {
					procCallMap.insert(pair<string, vector<pair<string, int>>>(currToken, { {procedureList.back(),stmtNum} }));
				}
				else {
					procCallMap[currToken].push_back({ procedureList.back(),stmtNum });
				}

				//merge and store container list onto map, for reference when handling parent* relation
				vector<pair<string, int>> mergedContainerList = containerList;
				mergedContainerList.insert(mergedContainerList.end(), procContMap[procedureList.back()].begin(), procContMap[procedureList.back()].end());
				if (procContMap.find(currToken) == procContMap.end()) {
					mergedContainerList.erase(mergedContainerList.begin());
					procContMap.insert(pair<string, vector<pair<string, int>>>(currToken, mergedContainerList));
				}
				else {
					mergedContainerList.insert(mergedContainerList.end(), procContMap[currToken].begin(), procContMap[currToken].end());
					procContMap[currToken] = mergedContainerList;
				}


			}
			else if (prevToken == "procedure") {
				//handle insertCalls
				insertForAllCalls(currToken, stmtNum, currToken, procCallMap);
				Database::insertProcedure(currToken);
				//procedureList.back() holds the current procedure that's being handled
				procedureList.push_back(currToken);
				procedureStart = stmtNum + 1;
				prevProcedure = procedureName;
				procedureName = currToken;
				//empty containerList
				containerList.clear();
				indPrevStmtNumList = {};
			}
			
		}
		
		//LOOP INIT//
		if (i+1 == tokens.size()) {
			if (repeated == false) {
				i = 2; //repeat to update parent
				repeated = true;
				stmtNum = 0;
				procedureList.push_back(tokens.at(1));
				prevStmtNum = 0;
				prevStmtNumList.clear();
				indPrevStmtNumList = {};
			}
			else {
				break;
			}
		}
	}
}		