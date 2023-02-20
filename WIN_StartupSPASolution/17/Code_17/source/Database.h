#pragma once

#include <string>
#include <vector>
#include "sqlite3.h"

using namespace std;

// The Database has to be a static class due to various constraints.
// It is advisable to just add the insert / get functions based on the given examples.
class Database {
public:
	// method to execute a query
	static void executeQuery(string query);

	// method to execute a query and map the database results
	static void executeQueryAndMapResults(vector<string>& results, string query);

	// method to connect to the database and initialize tables in the database
	static void initialize();

	// method to close the database connection
	static void close();

	// method to insert/get a procedure into the database
	static void insertProcedure(string procedureName);
	static void getProcedures(vector<string>& results);

	// method to insert/get a variable into the database
	static void insertVariable(string variableName);
	static void getVariables(vector<string>& results);

	// method to insert/get a Constant into the database
	static void insertConstant(string constantName);
	static void getConstants(vector<string>& results);

	// method to insert/get a Assignment into the database
	static void insertAssignment(string assignmentName);
	static void getAssignments(vector<string>& results);

	// method to insert/get a Print into the database
	static void insertPrint(string printName);
	static void getPrints(vector<string>& results);

	// method to insert/get a Read into the database
	static void insertRead(string readName);
	static void getReads(vector<string>& results);

	// method to insert/get a Statement into the database
	static void insertStmt(string stmtno);
	static void getStmts(vector<string>& results);

	// method to insert a if into the database
	static void insertIf(string stmtno);

	// method to insert a while into the database
	static void insertWhile(string stmtno);

	// method to insert a pattern into the database
	static void insertPattern(string stmtNo, string source, string target);

	// method to insert modifies into the database
	static void insertModifies(string stmtno, string procedureName, string target);

	// method to insert uses into the database
	static void insertUses(string stmtno, string procedureName, string target);

	static void getQueryResults(vector<string>& results, string query);

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
