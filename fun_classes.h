/*******************************************************************************
 * fun_classes.h - CLASS-Level Aggregation Module
 *
 * Master equation pattern: Class_Employed_Count iterates each CLASS's
 * HOUSEHOLD objects ONCE, accumulating all 59 class-level values simultaneously.
 * Individual equations are EQUATION_DUMMY, computed by the master via WRITE().
 *
 * Naming convention: Household_X → Class_X (sums implied)
 *                    Averages use Class_Avg_X
 *
 * CLASS hierarchy:
 * - CLASS (class_id=0): Workers
 * - CLASS (class_id=1): Capitalists
 *
 * REQUIRES: LSD structure with CLASSES container and CLASS objects
 *           fun_households.h must be included before this file
 ******************************************************************************/

#ifndef FUN_CLASSES_H
#define FUN_CLASSES_H


/*============================================================================
 * MASTER AGGREGATION EQUATION
 *
 * Single CYCLE through all HOUSEHOLD objects in this CLASS.
 * Accumulates 59 values simultaneously: 50 sums, 7 averages, 2 counts.
 * All other Class_* equations are EQUATION_DUMMY pointing to this master.
 *============================================================================*/

EQUATION("Class_Employed_Count")
/*
Master aggregation equation for all class-level household aggregates.
Returns the number of employed households; WRITEs all other class totals.
*/
	// SUM accumulators — income
	double wage = 0, profit = 0, unemp_ben = 0, gross = 0, tax = 0;
	double disp = 0, real_disp = 0, avg_real_inc = 0, avg_nom_inc = 0;
	double transfer = 0, dep_ret = 0;
	// SUM accumulators — consumption
	double real_dom_dem = 0, real_imp_dem = 0, eff_exp = 0;
	double real_auton = 0, real_des_dom = 0, des_exp = 0;
	double eff_real_dom = 0, eff_real_imp = 0;
	double ret_dep = 0, int_funds = 0, max_exp = 0, asset_purch = 0;
	// SUM accumulators — financial stocks
	double deposits = 0, loans = 0, fin_assets = 0, net_wealth = 0, savings = 0;
	// SUM accumulators — financial flows
	double int_pay = 0, debt_pay = 0, dem_loans = 0, eff_loans = 0;
	double max_loans = 0, fin_oblig = 0;
	// SUM accumulators — wealth tax
	double wt_owed = 0, wt_pay = 0, wt_dep = 0, wt_assets = 0;
	double wt_borrow = 0, wt_buffer = 0;
	// SUM accumulators — evasion & capital flight
	double off_dep = 0, dom_dep = 0, undecl = 0, decl = 0, repat = 0;
	double asset_pen = 0, off_pen = 0, audited = 0, flight = 0, evasion = 0;
	// AVE accumulators (sum first, divide by i at end)
	double imp_share = 0, int_rate = 0, max_debt = 0;
	double propensity = 0, sav_rate = 0, debt_rate = 0, ref_inc = 0;
	// Count accumulators
	double employed = 0, unemployed = 0;
	// Derived accumulators (for country-level delegation)
	double empl_skill  = 0;   // sum of skill for employed workers (Country_Total_Employed_Skill)
	double wt_count    = 0;   // count of households paying wealth tax (Country_Wealth_Tax_Taxpayer_Count)
	double evader_cnt  = 0;   // count of evading households (Country_Evader_Count)
	double wt_debt_sum = 0;   // Σ(debt_rate_i × income_i) for Country_Debt_Rate
	i = 0;

	double this_class = V("class_id");  // 0=workers, 1=capitalists

	CYCLE(cur, "HOUSEHOLD")
	{
		double emp = VS(cur, "Household_Employment_Status");
		if(emp > 0) employed++;
		else        unemployed++;

		// employed skill (workers only — capitalists always have class_id=1)
		if(this_class == 0 && emp > 0)
			empl_skill += VS(cur, "household_skill");

		// income
		wage         += VS(cur, "Household_Wage_Income");
		profit       += VS(cur, "Household_Profit_Income");
		unemp_ben    += VS(cur, "Household_Unemployment_Benefits");
		gross        += VS(cur, "Household_Nominal_Gross_Income");
		tax          += VS(cur, "Household_Income_Taxation");
		disp         += VS(cur, "Household_Nominal_Disposable_Income");
		real_disp    += VS(cur, "Household_Real_Disposable_Income");
		avg_real_inc += VS(cur, "Household_Avg_Real_Income");
		avg_nom_inc  += VS(cur, "Household_Avg_Nominal_Income");
		transfer     += VS(cur, "Household_Transfer_Received");
		dep_ret      += VS(cur, "Household_Deposits_Return");

		// consumption
		real_dom_dem += VS(cur, "Household_Real_Domestic_Consumption_Demand");
		real_imp_dem += VS(cur, "Household_Real_Desired_Imported_Consumption");
		eff_exp      += VS(cur, "Household_Effective_Expenses");
		real_auton   += VS(cur, "Household_Real_Autonomous_Consumption");
		real_des_dom += VS(cur, "Household_Real_Desired_Domestic_Consumption");
		des_exp      += VS(cur, "Household_Desired_Expenses");
		eff_real_dom += VS(cur, "Household_Effective_Real_Domestic_Consumption");
		eff_real_imp += VS(cur, "Household_Effective_Real_Imported_Consumption");
		ret_dep      += VS(cur, "Household_Retained_Deposits");
		int_funds    += VS(cur, "Household_Internal_Funds");
		max_exp      += VS(cur, "Household_Maximum_Expenses");
		asset_purch  += VS(cur, "Household_Asset_Purchases");

		// financial stocks
		deposits   += VS(cur, "Household_Stock_Deposits");
		loans      += VS(cur, "Household_Stock_Loans");
		fin_assets += VS(cur, "Household_Financial_Assets");
		net_wealth += VS(cur, "Household_Net_Wealth");
		savings    += VS(cur, "Household_Savings");

		// weighted debt rate contribution (for Country_Debt_Rate)
		{ double dep_i = VS(cur, "Household_Stock_Deposits");
		  double lon_i = VS(cur, "Household_Stock_Loans");
		  double dis_i = VS(cur, "Household_Nominal_Disposable_Income");
		  double dr = (dep_i > 0.001) ? lon_i / dep_i : 0;
		  wt_debt_sum += dr * dis_i; }

		// financial flows
		int_pay   += VS(cur, "Household_Interest_Payment");
		debt_pay  += VS(cur, "Household_Debt_Payment");
		dem_loans += VS(cur, "Household_Demand_Loans");
		eff_loans += VS(cur, "Household_Effective_Loans");
		max_loans += VS(cur, "Household_Max_Loans");
		fin_oblig += VS(cur, "Household_Financial_Obligations");

		// wealth tax
		wt_owed   += VS(cur, "Household_Wealth_Tax_Owed");
		{ double wt_p = VS(cur, "Household_Wealth_Tax_Payment"); wt_pay += wt_p; if(wt_p > 0.01) wt_count++; }
		wt_dep    += VS(cur, "Household_Wealth_Tax_From_Deposits");
		wt_assets += VS(cur, "Household_Wealth_Tax_From_Assets");
		wt_borrow += VS(cur, "Household_Wealth_Tax_From_Borrowing");
		wt_buffer += VS(cur, "Household_Wealth_Tax_From_Buffer");

		// evasion & capital flight
		{ double off_i = VS(cur, "Household_Deposits_Offshore"); off_dep += off_i;
		  double und_i = VS(cur, "Household_Assets_Undeclared"); undecl  += und_i;
		  if(off_i > 0.01 || und_i > 0.01) evader_cnt++; }
		dom_dep   += VS(cur, "Household_Deposits_Domestic");
		decl      += VS(cur, "Household_Assets_Declared");
		repat     += VS(cur, "Household_Repatriated_Deposits");
		asset_pen += VS(cur, "Household_Asset_Penalty");
		off_pen   += VS(cur, "Household_Offshore_Penalty");
		audited   += VS(cur, "Household_Is_Audited");
		flight    += VS(cur, "Household_Decision_Flight");
		evasion   += VS(cur, "Household_Decision_Evasion");

		// averages (sum here; divide by i below)
		imp_share  += VS(cur, "Household_Imports_Share");
		int_rate   += VS(cur, "Household_Interest_Rate");
		max_debt   += VS(cur, "Household_Max_Debt_Rate");
		propensity += VS(cur, "Household_Propensity_to_Spend");
		sav_rate   += VS(cur, "Household_Savings_Rate");
		debt_rate  += VS(cur, "Household_Debt_Rate");
		ref_inc    += VS(cur, "Household_Reference_Income");

		i++;
	}

	// Write derived counts/sums (delegated from country-level equations)
	WRITE("Class_Employed_Skill",          empl_skill);
	WRITE("Class_Wealth_Tax_Payer_Count",  wt_count);
	WRITE("Class_Evader_Count",            evader_cnt);
	WRITE("Class_Weighted_Debt_Sum",       wt_debt_sum);

	// Write counts
	WRITE("Class_Unemployed_Count", unemployed);

	// Write sums — income
	WRITE("Class_Wage_Income",               wage);
	WRITE("Class_Profit_Income",             profit);
	WRITE("Class_Unemployment_Benefits",     unemp_ben);
	WRITE("Class_Nominal_Gross_Income",      gross);
	WRITE("Class_Income_Taxation",           tax);
	WRITE("Class_Nominal_Disposable_Income", disp);
	WRITE("Class_Real_Disposable_Income",    real_disp);
	WRITE("Class_Avg_Real_Income",           avg_real_inc);
	WRITE("Class_Avg_Nominal_Income",        avg_nom_inc);
	WRITE("Class_Transfer_Received",         transfer);
	WRITE("Class_Deposits_Return",           dep_ret);

	// Write sums — consumption
	WRITE("Class_Real_Domestic_Consumption_Demand",    real_dom_dem);
	WRITE("Class_Real_Desired_Imported_Consumption",   real_imp_dem);
	WRITE("Class_Effective_Expenses",                  eff_exp);
	WRITE("Class_Real_Autonomous_Consumption",         real_auton);
	WRITE("Class_Real_Desired_Domestic_Consumption",   real_des_dom);
	WRITE("Class_Desired_Expenses",                    des_exp);
	WRITE("Class_Effective_Real_Domestic_Consumption", eff_real_dom);
	WRITE("Class_Effective_Real_Imported_Consumption", eff_real_imp);
	WRITE("Class_Retained_Deposits",                   ret_dep);
	WRITE("Class_Internal_Funds",                      int_funds);
	WRITE("Class_Maximum_Expenses",                    max_exp);
	WRITE("Class_Asset_Purchases",                     asset_purch);

	// Write sums — financial stocks
	WRITE("Class_Stock_Deposits",   deposits);
	WRITE("Class_Stock_Loans",      loans);
	WRITE("Class_Financial_Assets", fin_assets);
	WRITE("Class_Net_Wealth",       net_wealth);
	WRITE("Class_Savings",          savings);

	// Write sums — financial flows
	WRITE("Class_Interest_Payment",      int_pay);
	WRITE("Class_Debt_Payment",          debt_pay);
	WRITE("Class_Demand_Loans",          dem_loans);
	WRITE("Class_Effective_Loans",       eff_loans);
	WRITE("Class_Max_Loans",             max_loans);
	WRITE("Class_Financial_Obligations", fin_oblig);

	// Write sums — wealth tax
	WRITE("Class_Wealth_Tax_Owed",           wt_owed);
	WRITE("Class_Wealth_Tax_Payment",        wt_pay);
	WRITE("Class_Wealth_Tax_From_Deposits",  wt_dep);
	WRITE("Class_Wealth_Tax_From_Assets",    wt_assets);
	WRITE("Class_Wealth_Tax_From_Borrowing", wt_borrow);
	WRITE("Class_Wealth_Tax_From_Buffer",    wt_buffer);

	// Write sums — evasion & capital flight
	WRITE("Class_Deposits_Offshore",    off_dep);
	WRITE("Class_Deposits_Domestic",    dom_dep);
	WRITE("Class_Assets_Undeclared",    undecl);
	WRITE("Class_Assets_Declared",      decl);
	WRITE("Class_Repatriated_Deposits", repat);
	WRITE("Class_Asset_Penalty",        asset_pen);
	WRITE("Class_Offshore_Penalty",     off_pen);
	WRITE("Class_Audited_Count",        audited);
	WRITE("Class_Flight_Count",         flight);
	WRITE("Class_Evasion_Count",        evasion);

	// Write averages (accumulated sum / household count)
	WRITE("Class_Avg_Imports_Share",       (i > 0) ? imp_share / i : 0);
	WRITE("Class_Avg_Interest_Rate",       (i > 0) ? int_rate   / i : 0);
	WRITE("Class_Avg_Max_Debt_Rate",       (i > 0) ? max_debt   / i : 0);
	WRITE("Class_Avg_Propensity_to_Spend", (i > 0) ? propensity / i : 0);
	WRITE("Class_Avg_Savings_Rate",        (i > 0) ? sav_rate   / i : 0);
	WRITE("Class_Avg_Debt_Rate",           (i > 0) ? debt_rate  / i : 0);
	WRITE("Class_Avg_Reference_Income",    (i > 0) ? ref_inc    / i : 0);

RESULT(employed)


/*============================================================================
 * EQUATION_DUMMY DECLARATIONS
 * All class aggregates are computed by Class_Employed_Count via WRITE().
 * These declarations preserve LSD's dependency resolution.
 *============================================================================*/

// Counts
EQUATION_DUMMY("Class_Unemployed_Count",        "Class_Employed_Count")
// Derived country-level delegates
EQUATION_DUMMY("Class_Employed_Skill",          "Class_Employed_Count")
EQUATION_DUMMY("Class_Wealth_Tax_Payer_Count",  "Class_Employed_Count")
EQUATION_DUMMY("Class_Evader_Count",            "Class_Employed_Count")
EQUATION_DUMMY("Class_Weighted_Debt_Sum",       "Class_Employed_Count")

// Income
EQUATION_DUMMY("Class_Wage_Income",               "Class_Employed_Count")
EQUATION_DUMMY("Class_Profit_Income",             "Class_Employed_Count")
EQUATION_DUMMY("Class_Unemployment_Benefits",     "Class_Employed_Count")
EQUATION_DUMMY("Class_Nominal_Gross_Income",      "Class_Employed_Count")
EQUATION_DUMMY("Class_Income_Taxation",           "Class_Employed_Count")
EQUATION_DUMMY("Class_Nominal_Disposable_Income", "Class_Employed_Count")
EQUATION_DUMMY("Class_Real_Disposable_Income",    "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Real_Income",           "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Nominal_Income",        "Class_Employed_Count")
EQUATION_DUMMY("Class_Transfer_Received",         "Class_Employed_Count")
EQUATION_DUMMY("Class_Deposits_Return",           "Class_Employed_Count")

// Consumption
EQUATION_DUMMY("Class_Real_Domestic_Consumption_Demand",    "Class_Employed_Count")
EQUATION_DUMMY("Class_Real_Desired_Imported_Consumption",   "Class_Employed_Count")
EQUATION_DUMMY("Class_Effective_Expenses",                  "Class_Employed_Count")
EQUATION_DUMMY("Class_Real_Autonomous_Consumption",         "Class_Employed_Count")
EQUATION_DUMMY("Class_Real_Desired_Domestic_Consumption",   "Class_Employed_Count")
EQUATION_DUMMY("Class_Desired_Expenses",                    "Class_Employed_Count")
EQUATION_DUMMY("Class_Effective_Real_Domestic_Consumption", "Class_Employed_Count")
EQUATION_DUMMY("Class_Effective_Real_Imported_Consumption", "Class_Employed_Count")
EQUATION_DUMMY("Class_Retained_Deposits",                   "Class_Employed_Count")
EQUATION_DUMMY("Class_Internal_Funds",                      "Class_Employed_Count")
EQUATION_DUMMY("Class_Maximum_Expenses",                    "Class_Employed_Count")
EQUATION_DUMMY("Class_Asset_Purchases",                     "Class_Employed_Count")

// Financial stocks
EQUATION_DUMMY("Class_Stock_Deposits",   "Class_Employed_Count")
EQUATION_DUMMY("Class_Stock_Loans",      "Class_Employed_Count")
EQUATION_DUMMY("Class_Financial_Assets", "Class_Employed_Count")
EQUATION_DUMMY("Class_Net_Wealth",       "Class_Employed_Count")
EQUATION_DUMMY("Class_Savings",          "Class_Employed_Count")

// Financial flows
EQUATION_DUMMY("Class_Interest_Payment",      "Class_Employed_Count")
EQUATION_DUMMY("Class_Debt_Payment",          "Class_Employed_Count")
EQUATION_DUMMY("Class_Demand_Loans",          "Class_Employed_Count")
EQUATION_DUMMY("Class_Effective_Loans",       "Class_Employed_Count")
EQUATION_DUMMY("Class_Max_Loans",             "Class_Employed_Count")
EQUATION_DUMMY("Class_Financial_Obligations", "Class_Employed_Count")

// Wealth tax
EQUATION_DUMMY("Class_Wealth_Tax_Owed",           "Class_Employed_Count")
EQUATION_DUMMY("Class_Wealth_Tax_Payment",        "Class_Employed_Count")
EQUATION_DUMMY("Class_Wealth_Tax_From_Deposits",  "Class_Employed_Count")
EQUATION_DUMMY("Class_Wealth_Tax_From_Assets",    "Class_Employed_Count")
EQUATION_DUMMY("Class_Wealth_Tax_From_Borrowing", "Class_Employed_Count")
EQUATION_DUMMY("Class_Wealth_Tax_From_Buffer",    "Class_Employed_Count")

// Evasion & capital flight
EQUATION_DUMMY("Class_Deposits_Offshore",    "Class_Employed_Count")
EQUATION_DUMMY("Class_Deposits_Domestic",    "Class_Employed_Count")
EQUATION_DUMMY("Class_Assets_Undeclared",    "Class_Employed_Count")
EQUATION_DUMMY("Class_Assets_Declared",      "Class_Employed_Count")
EQUATION_DUMMY("Class_Repatriated_Deposits", "Class_Employed_Count")
EQUATION_DUMMY("Class_Asset_Penalty",        "Class_Employed_Count")
EQUATION_DUMMY("Class_Offshore_Penalty",     "Class_Employed_Count")
EQUATION_DUMMY("Class_Audited_Count",        "Class_Employed_Count")
EQUATION_DUMMY("Class_Flight_Count",         "Class_Employed_Count")
EQUATION_DUMMY("Class_Evasion_Count",        "Class_Employed_Count")

// Averages
EQUATION_DUMMY("Class_Avg_Imports_Share",       "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Interest_Rate",       "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Max_Debt_Rate",       "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Propensity_to_Spend", "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Savings_Rate",        "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Debt_Rate",           "Class_Employed_Count")
EQUATION_DUMMY("Class_Avg_Reference_Income",    "Class_Employed_Count")


/*============================================================================
 * UNCHANGED EQUATIONS
 * These don't iterate households — O(1) count or derived from master outputs.
 *============================================================================*/

EQUATION("Class_Household_Count")
/*
Total number of households in this class.
*/
RESULT(COUNT("HOUSEHOLD"))


EQUATION("Class_Avg_Disposable_Income")
/*
Average disposable income per household in this class.
*/
v[0] = V("Class_Nominal_Disposable_Income");
v[1] = V("Class_Household_Count");
RESULT((v[1] > 0) ? v[0] / v[1] : 0)


EQUATION("Class_Avg_Net_Wealth")
/*
Average net wealth per household in this class.
*/
v[0] = V("Class_Net_Wealth");
v[1] = V("Class_Household_Count");
RESULT((v[1] > 0) ? v[0] / v[1] : 0)


EQUATION("Class_Income_Share")
/*
This class's share of total household disposable income.
NOTE: Explicitly sums both classes because SUMS(country, ...) only finds first CLASSES.
*/
v[0] = V("Class_Nominal_Disposable_Income");
v[1] = VS(working_class, "Class_Nominal_Disposable_Income") + VS(capitalist_class, "Class_Nominal_Disposable_Income");
RESULT((v[1] > 0) ? v[0] / v[1] : 0)


EQUATION("Class_Wealth_Share")
/*
This class's share of total household net wealth.
NOTE: Explicitly sums both classes because SUMS(country, ...) only finds first CLASSES.
*/
v[0] = V("Class_Net_Wealth");
v[1] = VS(working_class, "Class_Net_Wealth") + VS(capitalist_class, "Class_Net_Wealth");
RESULT((v[1] > 0) ? v[0] / v[1] : 0)


EQUATION("Class_Consumption_Share")
/*
This class's share of total household consumption.
NOTE: Explicitly sums both classes because SUMS(country, ...) only finds first CLASSES.
*/
v[0] = V("Class_Effective_Expenses");
v[1] = VS(working_class, "Class_Effective_Expenses") + VS(capitalist_class, "Class_Effective_Expenses");
RESULT((v[1] > 0) ? v[0] / v[1] : 0)


#endif // FUN_CLASSES_H
