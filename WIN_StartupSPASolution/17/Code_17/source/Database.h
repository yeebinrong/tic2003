#pragma once

#include <string>
#include <vector>
#include "sqlite3.h"

using namespace std;

// The Database has to be a static class due to various constraints.
// It is advisable to just add the insert / get functions based on the given examples.
class Database {
public:
	// method to connect to the database and initialize tables in the database
	static void initialize();

	// method to close the database connection
	static void close();

	// method to insert a procedure into the database
	static void insertProcedure(string procedureName);

	// method to get all the procedures from the database
	static void getProcedures(vector<string>& results);

	// method to insert a variable into the database
	static void insertVariable(string variableName);

	// method to get all the variable from the database
	static void getVariables(vector<string>& results);

	// method to insert a Constant into the database
	static void insertConstant(string constantName);

	// method to get all the Constant from the database
	static void getConstants(vector<string>& results);

	// method to insert a Assignment into the database
	static void insertAssignment(string assignmentName);

	// method to get all the Assignment from the database
	static void getAssignments(vector<string>& results);

	// method to insert a Print into the database
	static void insertPrint(string printName);

	// method to get all the Print from the database
	static void getPrints(vector<string>& results);

	// method to insert a Read into the database
	static void insertRead(string readName);

	// method to get all the Read from the database
	static void getReads(vector<string>& results);

	// method to insert a Statement into the database
	static void insertStmt(string stmtName);

	// method to get all the Statement from the database
	static void getStmts(vector<string>& results);

	// method to insert a Next into the database
	static void insertNext(string stmtNo, string nextStmtNo, string direct);

	// method to get all the Nexts from the database
	static void getNexts(vector<string>& results);

		// method to insert a Parent into the database
	static void insertParent(string stmtNo, string childStmtNo, string direct );

	// method to get all the Parents from the database
	static void getParents(vector<string>& results);

private:
	// the connection pointer to the database
	static sqlite3* dbConnection;
	// a vector containing the results from the database
	static vector<vector<string>> dbResults;
	// the error message from the database
	static char* errorMessage;
	// callback method to put one row of results from the database into the dbResults vector
	// This method is called each time a row of results is returned from the database
	static int callback(void* NotUsed, int argc, char** argv, char** azColName);
};
