/***********ANALYSIS VARIABLES************/

/*****COUNTRY STATS*****/

EQUATION("GDP")//Quarterly Nominal GDP
RESULT(VS(country, "Country_GDP"))

EQUATION("P")//Price Index
RESULT(VS(country, "Country_Price_Index"))

EQUATION("CPI")//Price Index
RESULT(VS(country, "Country_Consumer_Price_Index"))

EQUATION("P_G")//Annual Inflation
RESULT(VS(country, "Country_Annual_Inflation"))

EQUATION("CPI_G")//Annual CPI Inflation
RESULT(VS(country, "Country_Annual_CPI_Inflation"))

EQUATION("U")//Unemployment
RESULT(VS(country, "Country_Idle_Capacity"))

EQUATION("EMP")//Employment
RESULT(SUMS(country, "Sector_Employment"))

EQUATION("GDP_G")//GDP real annual growth rate
RESULT(VS(country, "Country_Annual_Real_Growth"))

EQUATION("G_n")//GDP nominal annual growth rate
RESULT(VS(country, "Country_Annual_Growth"))

EQUATION("Cri")//Crisis counters
RESULT(VS(country, "Country_Likelihood_Crisis"))

EQUATION("C")//Quarterly Nominal Consumption
RESULT(VS(country, "Country_Total_Household_Expenses"))

EQUATION("I")//Quarterly Nominal Investment
RESULT(VS(country, "Country_Total_Investment_Expenses"))

EQUATION("PROD")//Average Productivity
RESULT(VS(country, "Country_Avg_Productivity"))

EQUATION("MK")//Average Markup
RESULT(VS(country, "Country_Avg_Markup"))

EQUATION("KL")//Capital labour ratio
RESULT(VS(country,"Country_Capital_Labor_Ratio"))

EQUATION("PR")//Profit Rate
RESULT(VS(country,"Country_Avg_Profit_Rate"))

EQUATION("PCU")//Productive Capacity Utilization Rate
RESULT(VS(country,"Country_Capacity_Utilization"))

EQUATION("PROFITS")//Total Profits
RESULT(VS(country, "Country_Total_Profits"))

EQUATION("WAGE")//Total Wages
RESULT(VS(country, "Country_Total_Wages"))

EQUATION("PSH")//Profit Share
RESULT(VS(country,"Country_Profit_Share"))

EQUATION("WSH")//Wage Share
RESULT(VS(country,"Country_Wage_Share"))

EQUATION("GINI")//Gini Index (Disposable Income, post-tax)
RESULT(VS(country,"Country_Gini_Index"))

EQUATION("GINI_PRETAX")//Gini Index (Gross Income, pre-tax)
RESULT(VS(country,"Country_Gini_Index_Pretax"))

EQUATION("GINI_W")//Gini Index (Net Wealth, post-tax)
RESULT(VS(country,"Country_Gini_Index_Wealth"))

EQUATION("GINI_W_PRETAX")//Gini Index (Net Wealth, pre-tax)
RESULT(VS(country,"Country_Gini_Index_Wealth_Pretax"))

/*****INCOME INEQUALITY INDICES (post-tax)*****/

EQUATION("PALMA")//Palma Ratio (Disposable Income)
RESULT(VS(country,"Country_Palma_Ratio_Income"))

EQUATION("TOP10")//Top 10% Share (Disposable Income)
RESULT(VS(country,"Country_Top10_Share_Income"))

EQUATION("TOP1")//Top 1% Share (Disposable Income)
RESULT(VS(country,"Country_Top1_Share_Income"))

EQUATION("BOT50")//Bottom 50% Share (Disposable Income)
RESULT(VS(country,"Country_Bottom50_Share_Income"))

/*****WEALTH INEQUALITY INDICES (post-tax)*****/

EQUATION("PALMA_W")//Palma Ratio (Net Wealth, post-tax)
RESULT(VS(country,"Country_Palma_Ratio_Wealth"))

EQUATION("TOP10_W")//Top 10% Share (Net Wealth, post-tax)
RESULT(VS(country,"Country_Top10_Share_Wealth"))

EQUATION("TOP1_W")//Top 1% Share (Net Wealth, post-tax)
RESULT(VS(country,"Country_Top1_Share_Wealth"))

EQUATION("BOT50_W")//Bottom 50% Share (Net Wealth, post-tax)
RESULT(VS(country,"Country_Bottom50_Share_Wealth"))

/*****WEALTH INEQUALITY INDICES (pre-tax)*****/

EQUATION("PALMA_W_PRETAX")//Palma Ratio (Net Wealth, pre-tax)
RESULT(VS(country,"Country_Palma_Ratio_Wealth_Pretax"))

EQUATION("TOP10_W_PRETAX")//Top 10% Share (Net Wealth, pre-tax)
RESULT(VS(country,"Country_Top10_Share_Wealth_Pretax"))

EQUATION("TOP1_W_PRETAX")//Top 1% Share (Net Wealth, pre-tax)
RESULT(VS(country,"Country_Top1_Share_Wealth_Pretax"))

EQUATION("BOT50_W_PRETAX")//Bottom 50% Share (Net Wealth, pre-tax)
RESULT(VS(country,"Country_Bottom50_Share_Wealth_Pretax"))


/*****REAL STATS*****/

EQUATION("GDP_r")//Real GDP
RESULT(VS(country, "Country_Real_GDP_Demand"))

EQUATION("C_r")//Quarterly Real Consumption
RESULT(VS(country, "Country_Total_Household_Expenses")/V("P"))
//RESULT(VS(country, "Country_Nominal_Consumption_Production")/V("P"))

EQUATION("I_r")//Quarterly Real Investment
RESULT(VS(country, "Country_Total_Investment_Expenses")/V("P"))
//RESULT(VS(country, "Country_Nominal_Capital_Production")/V("P"))

EQUATION("INVE_r")//Real Aggregate Inventories
RESULT(VS(country, "Country_Inventories")/V("P"))

EQUATION("K_r")//Real Stock of Capital
RESULT(VS(country, "Country_Capital_Stock")/V("P"))

EQUATION("G_r")//Quarterly Real Government Expenses
RESULT(VS(government, "Government_Effective_Expenses")/V("P"))

EQUATION("PROF_r")//Real Profits
RESULT(VS(country, "Country_Total_Profits")/V("P"))

EQUATION("WAGE_r")//Real Wages
RESULT(VS(country, "Country_Total_Wages")/V("P"))

EQUATION("M_r")//Quarterly Real Imports
RESULT(VS(country, "Country_Nominal_Imports")/V("P"))

EQUATION("X_r")//Quarterly Real Exports
RESULT(VS(country, "Country_Nominal_Exports")/V("P"))

EQUATION("NX_r")//Quarterly Real Net Exports
RESULT(V("X_r")-V("M_r"))


/*****FINANCIAL STATS*****/

EQUATION("DEBT_RT_C")//Average Debt Rate of Consumption good sector
RESULT(VS(consumption, "Sector_Avg_Debt_Rate"))

EQUATION("DEBT_RT_K")//Average Debt Rate of Capital good sector
RESULT(VS(capital, "Sector_Avg_Debt_Rate"))

EQUATION("DEBT_RT_I")//Average Debt Rate of Intermediate good sector
RESULT(VS(input, "Sector_Avg_Debt_Rate"))

EQUATION("DEBT_RT_FI")//Average Debt Rate of all firms
RESULT(VS(country, "Country_Debt_Rate_Firms"))

EQUATION("DEBT_RT_HH")//Average Debt Rate of all households
RESULT(VS(country, "Country_Debt_Rate"))

EQUATION("DEBT_FS_ST")//Stock of short term debt in the financial sector
RESULT(VS(financial, "Financial_Sector_Stock_Loans_Short_Term"))

EQUATION("DEBT_FS_LT")//Stock of long term debt in the financial sector
RESULT(VS(financial, "Financial_Sector_Stock_Loans_Long_Term"))

EQUATION("DEBT_FS")//Stock of total debt in the financial sector
RESULT(VS(financial, "Financial_Sector_Total_Stock_Loans"))

EQUATION("DEP_FS")//Stock of total deposits in the financial sector
RESULT(VS(financial, "Financial_Sector_Stock_Deposits"))

EQUATION("FS_STR")//Financial sector short term rate
RESULT(VS(financial, "Financial_Sector_Short_Term_Rate"))

EQUATION("FS_LEV")//Financial sector leverage
RESULT(VS(financial, "Financial_Sector_Leverage"))

EQUATION("FS_HHI")//Financial sector HHI
RESULT(VS(financial, "Financial_Sector_Normalized_HHI"))

EQUATION("FS_DR")//Financial sector Default rate
RESULT(VS(financial, "Financial_Sector_Default_Rate"))

EQUATION("FS_DEF")//Financial sector Default
RESULT(VS(financial, "Financial_Sector_Defaulted_Loans"))

EQUATION("FS_DMET")//Financial sector Demand Met
RESULT(VS(financial, "Financial_Sector_Demand_Met"))

EQUATION("FS_RES")//Financial sector Rescue
RESULT(VS(financial, "Financial_Sector_Rescue"))
   
EQUATION("FS_PR")//Financial sector profits
RESULT(VS(financial, "Financial_Sector_Profits"))   

EQUATION("PONZI")//Share of Firms in Ponzi position
RESULT(VS(country, "Country_Ponzi_Share"))    

EQUATION("SPEC")//Share of Firms in Speculative position
RESULT(VS(country, "Country_Speculative_Share"))  

EQUATION("HEDGE")//Share of Firms in Hedge position
RESULT(VS(country, "Country_Hedge_Share")) 

EQUATION("IR")//Basic Interest Rate
RESULT(VS(financial, "Central_Bank_Basic_Interest_Rate"))     

EQUATION("IR_DEP")//Interest Rate on Deposits
RESULT(VS(financial, "Financial_Sector_Interest_Rate_Deposits"))  

EQUATION("IR_ST")//Interest Rate on Short Term Loans
RESULT(VS(financial, "Financial_Sector_Avg_Interest_Rate_Short_Term"))  

EQUATION("IR_LT")//Interest Rate on Long Term Loans
RESULT(VS(financial, "Financial_Sector_Avg_Interest_Rate_Long_Term"))   

EQUATION("BKR")//Number of Bankrupt Events
RESULT(VS(country, "Exit_Bankruptcy_Events")) 

EQUATION("BKR_RT")//Bankrupt Rate
RESULT(VS(country, "Exit_Bankruptcy_Share")) 

/*****HOUSEHOLD TYPE STATS*****/
EQUATION("YSH_w")
// Worker income share = Class_Nominal_Disposable_Income(workers) / total
	v[0] = VS(working_class, "Class_Nominal_Disposable_Income");
	v[1] = v[0] + VS(capitalist_class, "Class_Nominal_Disposable_Income");
RESULT(v[1] > 0 ? v[0] / v[1] : 0)

EQUATION("YSH_c")
// Capitalist income share = Class_Nominal_Disposable_Income(capitalists) / total
	v[0] = VS(capitalist_class, "Class_Nominal_Disposable_Income");
	v[1] = v[0] + VS(working_class, "Class_Nominal_Disposable_Income");
RESULT(v[1] > 0 ? v[0] / v[1] : 0)

EQUATION("WSH_w")
// Worker deposit share = Class_Stock_Deposits(workers) / total
	v[0] = VS(working_class, "Class_Stock_Deposits");
	v[1] = v[0] + VS(capitalist_class, "Class_Stock_Deposits");
RESULT(v[1] > 0 ? v[0] / v[1] : 0)

EQUATION("WSH_c")
// Capitalist deposit share = Class_Stock_Deposits(capitalists) / total
	v[0] = VS(capitalist_class, "Class_Stock_Deposits");
	v[1] = v[0] + VS(working_class, "Class_Stock_Deposits");
RESULT(v[1] > 0 ? v[0] / v[1] : 0)

EQUATION("NW_w")
// Total worker net wealth
RESULT(VS(working_class, "Class_Net_Wealth"))

EQUATION("NW_c")
// Total capitalist net wealth
RESULT(VS(capitalist_class, "Class_Net_Wealth"))

EQUATION("NW_w_G")
// Growth rate of worker net wealth
RESULT(LAG_GROWTH(p, "NW_w", 1))

EQUATION("NW_c_G")
// Growth rate of capitalist net wealth
RESULT(LAG_GROWTH(p, "NW_c", 1))


/*****SECTORAL STATS*****/

EQUATION("P_C")//Average Price of Consumption good secto
RESULT(VS(consumption, "Sector_Avg_Price"))

EQUATION("P_K")//Average Price of Capital good sector
RESULT(VS(capital, "Sector_Avg_Price"))

EQUATION("P_I")//Average Price of Intermediate good sector
RESULT(VS(input, "Sector_Avg_Price"))

EQUATION("PX_C")//Average External Price of Consumption good secto
RESULT(VS(consumption, "Sector_External_Price"))

EQUATION("PX_K")//Average External Price of Capital good sector
RESULT(VS(capital, "Sector_External_Price"))

EQUATION("PX_I")//Average External Price of Intermediate good sector
RESULT(VS(input, "Sector_External_Price"))

EQUATION("W_C")//Average Wage of Consumption good sector
RESULT(VS(consumption, "Sector_Avg_Wage"))

EQUATION("W_K")//Average Wage of Capital good sector
RESULT(VS(capital, "Sector_Avg_Wage"))

EQUATION("W_I")//Average Wage of Intermediate good sector
RESULT(VS(input, "Sector_Avg_Wage"))

EQUATION("MK_C")//Average Markup of Consumption good sector
RESULT(VS(consumption, "Sector_Avg_Markup"))

EQUATION("MK_K")//Average Markup of Capital good sector
RESULT(VS(capital, "Sector_Avg_Markup"))

EQUATION("MK_I")//Average Markup of Intermediate good sector
RESULT(VS(input, "Sector_Avg_Markup"))

EQUATION("PROD_C")//Average Productivity of Consumption good sector
RESULT(VS(consumption, "Sector_Avg_Productivity"))

EQUATION("PROD_K")//Average Productivity of Capital good sector
RESULT(VS(capital, "Sector_Avg_Productivity"))

EQUATION("PROD_I")//Average Productivity of Intermediate good sector
RESULT(VS(input, "Sector_Avg_Productivity"))

EQUATION("U_C")//Unemployment Rate of Consumption good sector
RESULT(VS(consumption, "Sector_Idle_Capacity"))

EQUATION("U_K")//Unemployment Rate of Capital good sector
RESULT(VS(capital, "Sector_Idle_Capacity"))

EQUATION("U_I")//Unemployment Rate of Intermediate good sector
RESULT(VS(input, "Sector_Idle_Capacity"))

EQUATION("HHI_C")//Inverse HHI of Consumption good sector
RESULT(VS(consumption, "Sector_Normalized_HHI"))

EQUATION("HHI_K")//Inverse HHI of Capital good sector
RESULT(VS(capital, "Sector_Normalized_HHI"))

EQUATION("HHI_I")//Inverse HHI of Intermediate good sector
RESULT(VS(input, "Sector_Normalized_HHI"))

EQUATION("IRST_C")//Average Short Term Interest Rate of Consumption good sector
RESULT(VS(consumption, "Sector_Avg_Interest_Rate_Short_Term"))

EQUATION("IRST_K")//Average Short Term Interest Rate of Capital good sector
RESULT(VS(capital, "Sector_Avg_Interest_Rate_Short_Term"))

EQUATION("IRST_I")//Average Short Term Interest Rate of Intermediate good sector
RESULT(VS(input, "Sector_Avg_Interest_Rate_Short_Term"))

EQUATION("IRLT_C")//Average Long Term Interest Rate of Consumption good sector
RESULT(VS(consumption, "Sector_Avg_Interest_Rate_Long_Term"))

EQUATION("IRLT_K")//Average Long Term Interest Rate of Capital good sector
RESULT(VS(capital, "Sector_Avg_Interest_Rate_Long_Term"))

EQUATION("IRLT_I")//Average Long Term Interest Rate of Intermediate good sector
RESULT(VS(input, "Sector_Avg_Interest_Rate_Long_Term"))

/*****COUNTRY GROWTH STATS*****/

EQUATION("EMP_G")//Quarterly Employment Growth rate
RESULT(LAG_GROWTH(p,"EMP",1))

EQUATION("CON_G")//Quarterly Real Consumption Growth rate
RESULT(LAG_GROWTH(p,"C_r",1))

EQUATION("INV_G")//Quarterly Real Investment Growth rate
RESULT(LAG_GROWTH(p,"I_r",1))

EQUATION("PROD_G")//Average Productivity Growth
RESULT(LAG_GROWTH(p,"PROD",1))

EQUATION("MK_G")//Average Markup Growth
RESULT(LAG_GROWTH(p,"MK",1))

EQUATION("INVE_G")//Real Aggregate Inventories Growth
RESULT(LAG_GROWTH(p,"INVE_r",1))

EQUATION("K_G")//Real Stock of Capital Growth
RESULT(LAG_GROWTH(p,"K_r",1))

EQUATION("PROFITS_G")//Real Profits Growth rate
RESULT(LAG_GROWTH(p,"PROFITS",1))

EQUATION("WAGE_G")//Real Wages growth rate
RESULT(LAG_GROWTH(p,"WAGE",1))

EQUATION("GOV_G")//Quarterly Real Government Expenses Growth rate
RESULT(LAG_GROWTH(p,"G_r",1))

EQUATION("PDEBT_G")//Public Debt Growth rate
RESULT(LAG_GROWTH(p,"PDEBT",1))

EQUATION("M_G")//Quarterly Real Imports Growth rate
RESULT(LAG_GROWTH(p,"M_r",1))

EQUATION("X_G")//Quarterly Real Exports Growth rate
RESULT(LAG_GROWTH(p,"X_r",1))

EQUATION("NX_G")//Quarterly Real Net Exports Growth rate
RESULT(LAG_GROWTH(p,"NX_r",1))


/*****MACRO SHARE STATS*****/

EQUATION("CGDP")
RESULT(V("Country_Total_Household_Expenses")/V("Country_GDP_Demand"))

EQUATION("IGDP")
RESULT(V("Country_Total_Investment_Expenses")/V("Country_GDP_Demand"))

EQUATION("GGDP")
RESULT(V("Government_Effective_Expenses")/V("Country_GDP_Demand"))

EQUATION("NXGDP")
RESULT((V("Country_Nominal_Exports")-V("Country_Nominal_Imports"))/V("Country_GDP_Demand"))

EQUATION("XGDP")
RESULT(V("Country_Nominal_Exports")/V("Country_GDP_Demand"))

EQUATION("MGDP")
RESULT(V("Country_Nominal_Imports")/V("Country_GDP_Demand"))

EQUATION("INVGDP")
RESULT(V("Country_Inventories")/V("Country_GDP_Demand"))

EQUATION("KGDP")
RESULT(V("Country_Capital_Stock")/V("Country_GDP_Demand")) 


/*****FINANCIAL GROWTH STATS*****/

EQUATION("DEBT_FS_ST_G")//Stock of short term debt growth in the financial sector
RESULT(LAG_GROWTH(p,"DEBT_FS_ST",1))

EQUATION("DEBT_FS_LT_G")//Stock of long term debt growth in the financial sector
RESULT(LAG_GROWTH(p,"DEBT_FS_LT",1))

EQUATION("DEBT_FS_G")//Stock of total debt growth in the financial sector
RESULT(LAG_GROWTH(p,"DEBT_FS",1))

EQUATION("DEP_FS_G")//Stock of total deposits growth in the financial sector
RESULT(LAG_GROWTH(p,"DEP_FS",1))


/*****GOVERNMENT STATS*****/

EQUATION("G")//Quarterly Government Expenses
RESULT(VS(government, "Government_Effective_Expenses"))

EQUATION("TT")//Quarterly Total Taxes
RESULT(VS(government, "Government_Total_Taxes"))

EQUATION("DT")//Quarterly Direct Taxes
RESULT(VS(government, "Government_Income_Taxes"))

EQUATION("IT")//Quarterly Indiret Taxes
RESULT(VS(government, "Government_Indirect_Taxes"))

EQUATION("PST")//Primary Surplus Target
RESULT(VS(government, "Government_Surplus_Rate_Target"))

EQUATION("PS_GDP")//Primary Surplus to GDP
RESULT(VS(government, "Government_Surplus_GDP_Ratio"))

EQUATION("PDEBT")//Public Debt
RESULT(VS(government, "Government_Debt"))

EQUATION("PDEBT_GDP")//Public Debt to GDP
RESULT(VS(government, "Government_Debt_GDP_Ratio"))


/*****EXTERNAL STATS*****/

EQUATION("GDPX_r")//Real External Income
RESULT(VS(external, "External_Real_Income"))

EQUATION("X")//Quarterly Nominal Exports
RESULT(VS(external, "Country_Nominal_Exports"))

EQUATION("M")//Quarterly Nominal Imports
RESULT(VS(external, "Country_Nominal_Imports"))

EQUATION("NX")//Quarterly Trade Balance
RESULT(VS(external, "Country_Trade_Balance"))

EQUATION("CF")//Quarterly Capital Flows
RESULT(VS(external, "Country_Capital_Flows"))

EQUATION("RES")//Stock of International Reserves
RESULT(VS(external, "Country_International_Reserves"))

EQUATION("RES_GDP")//Stock of International Reserves to GDP
RESULT(VS(external, "Country_International_Reserves_GDP_Ratio"))

EQUATION("DX_GDP")//Stock of International Reserves to GDP
RESULT(VS(external, "Country_External_Debt_GDP_Ratio"))

EQUATION("ER")//Exchange Rate
RESULT(VS(external, "Country_Exchange_Rate"))


/*****STAGE 7: WEALTH TAX ANALYSIS*****/

EQUATION("WTAX")//Wealth Tax Revenue
RESULT(VS(country, "Country_Wealth_Tax_Revenue"))

EQUATION("WTAX_DEP")//Wealth Tax From Deposits
RESULT(VS(country, "Country_Wealth_Tax_From_Deposits"))

EQUATION("WTAX_AST")//Wealth Tax From Asset Liquidation
RESULT(VS(country, "Country_Wealth_Tax_From_Assets"))

EQUATION("WTAX_BOR")//Wealth Tax From Borrowing
RESULT(VS(country, "Country_Wealth_Tax_From_Borrowing"))

EQUATION("WTAX_COUNT")//Wealth Tax Taxpayer Count
RESULT(VS(country, "Country_Wealth_Tax_Taxpayer_Count"))


EQUATION("Test_Wealth_Tax_Consistency")
/*
Stage 7: Verify wealth tax consistency:
1. Government revenue = SUM(household payments)
2. Payment sources sum to total payment
3. Only households above threshold pay
*/
v[0] = VS(country, "switch_class_tax_structure");
if(v[0] < 5)
{
    v[12] = 0;  // Wealth tax inactive, no errors
}
else
{
    // Check 1: Government revenue = household sum
    v[1] = VS(government, "Government_Wealth_Tax_Revenue");
    v[2] = SUMS(working_class, "Household_Wealth_Tax_Payment") + SUMS(capitalist_class, "Household_Wealth_Tax_Payment");
    v[3] = fabs(v[1] - v[2]);

    // Check 2: Payment sources sum to total
    v[4] = VS(country, "Country_Wealth_Tax_From_Deposits");
    v[5] = VS(country, "Country_Wealth_Tax_From_Assets");
    v[6] = VS(country, "Country_Wealth_Tax_From_Borrowing");
    v[7] = fabs((v[4] + v[5] + v[6]) - v[2]);

    // Check 3: Threshold logic (count errors)
    v[8] = VS(country, "wealth_tax_threshold");
    v[9] = 0;  // Error count
    CYCLE(cur1, "CLASSES")
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        v[10] = VLS(cur, "Household_Net_Wealth", 1);  // Lagged wealth (tax base)
        v[11] = VS(cur, "Household_Wealth_Tax_Payment");

        // Error if: below threshold but paid, OR above threshold but didn't pay
        if(v[10] < v[8] && v[11] > 0.01) v[9]++;
        if(v[10] > v[8] && v[11] < 0.01) v[9]++;
    }
    }

    // Total error metric
    v[12] = v[3] + v[7] + v[9];

    if(v[12] > 0.1)
        PLOG("\n[Stage 7] WARNING: Wealth tax errors at t=%d (err=%.4f)", t, v[12]);
}

RESULT(v[12])


/*****STAGE 9: TAX EVASION & CAPITAL FLIGHT ANALYSIS*****/

EQUATION("EVADE_OFFSHORE")//Offshore Deposits (Capital Flight)
RESULT(VS(country, "Country_Total_Deposits_Offshore"))

EQUATION("EVADE_UNDECLARED")//Undeclared Financial Assets
RESULT(VS(country, "Country_Total_Assets_Undeclared"))

EQUATION("EVADE_DECLARED")//Declared Financial Assets
RESULT(VS(country, "Country_Total_Assets_Declared"))

EQUATION("EVADE_FLIGHT_RATE")//Capital Flight Rate (Offshore/Total Deposits)
RESULT(VS(country, "Country_Capital_Flight_Rate"))

EQUATION("EVADE_ASSET_RATE")//Asset Evasion Rate (Undeclared/Total Assets)
RESULT(VS(country, "Country_Asset_Evasion_Rate"))

EQUATION("EVADE_COUNT")//Number of Evaders
RESULT(VS(country, "Country_Evader_Count"))

EQUATION("EVADE_AUDITS")//Number of Audits
RESULT(VS(country, "Country_Audit_Count"))

EQUATION("EVADE_PENALTIES")//Penalty Revenue
RESULT(VS(country, "Country_Penalty_Revenue"))

EQUATION("EVADE_TAX_GAP")//Revenue Lost to Evasion
RESULT(VS(government, "Government_Wealth_Tax_Gap"))

EQUATION("EVADE_DYNAMIC_AUDIT")//Dynamic Audit Probability
RESULT(VS(government, "Government_Dynamic_Audit_Probability"))


EQUATION("Test_Evasion_Consistency")
/*
Stage 9: Verify evasion consistency:
1. Offshore + Domestic = Total Deposits
2. Declared + Undeclared = Total Financial Assets
3. Penalty revenue = SUM(household penalties)
*/
v[0] = VS(country, "switch_class_tax_structure");
if(v[0] < 5)
{
    v[12] = 0;  // Wealth tax/evasion inactive
}
else
{
    // Check 1: Deposit split (Offshore + Domestic = Total)
    v[1] = VS(country, "Country_Total_Deposits_Offshore");
    v[2] = SUMS(working_class, "Household_Deposits_Domestic") + SUMS(capitalist_class, "Household_Deposits_Domestic");
    v[3] = VS(country, "Country_Total_Household_Stock_Deposits");
    v[4] = fabs((v[1] + v[2]) - v[3]);

    // Check 2: Asset split (Declared + Undeclared = Total)
    v[5] = VS(country, "Country_Total_Assets_Declared");
    v[6] = VS(country, "Country_Total_Assets_Undeclared");
    v[7] = SUMS(working_class, "Household_Financial_Assets") + SUMS(capitalist_class, "Household_Financial_Assets");
    v[8] = fabs((v[5] + v[6]) - v[7]);

    // Check 3: Penalty revenue = SUM(Asset_Penalty)
    v[9] = VS(country, "Country_Penalty_Revenue");
    v[10] = SUMS(working_class, "Household_Asset_Penalty") + SUMS(capitalist_class, "Household_Asset_Penalty");
    v[11] = fabs(v[9] - v[10]);

    // Total error metric
    v[12] = v[4] + v[8] + v[11];

    if(v[12] > 0.1)
        PLOG("\n[Stage 9] WARNING: Evasion consistency errors at t=%d (err=%.4f)", t, v[12]);
}

RESULT(v[12])


/*****PHASE 1: CLASS STRUCTURE VERIFICATION*****/

EQUATION("Test_Class_Household_Integrity")
/*
Phase 1: Verify CLASS structure integrity.
Checks that household_type matches parent CLASS class_id:
- class_id=0 (working_class) should contain household_type=0
- class_id=1 (capitalist_class) should contain household_type=1

Returns: Error count (should be 0)
*/
v[0] = 0;  // error count

// Only run if CLASS structure is initialized
if(working_class != NULL && capitalist_class != NULL)
{
    CYCLE(cur1, "CLASSES")
    {
        v[1] = VS(cur1, "class_id");  // expected household_type (same value)

        CYCLES(cur1, cur2, "HOUSEHOLD")
        {
            v[2] = VS(cur2, "household_type");
            if(v[2] != v[1])
            {
                v[0]++;
                if(v[0] <= 5)  // Log first 5 errors only
                    PLOG("\n[Phase 1] ERROR: Household %g has type %.0f but is in CLASS %.0f",
                         VS(cur2, "household_id"), v[2], v[1]);
            }
        }
    }

    if(v[0] > 0)
        PLOG("\n[Phase 1] WARNING: %g households in wrong CLASS", v[0]);
}

RESULT(v[0])


EQUATION("Test_Class_Aggregation_Identity")
/*
Phase 1: Verify CLASS aggregation identity.
Checks that SUM(Class_X) = SUMS(households, Household_X)

Returns: Relative error (should be < 0.001)
*/
v[0] = 0;  // Sum via CLASS equations
v[1] = 0;  // Sum via direct household aggregation

// Only run if CLASS structure is initialized
if(working_class != NULL && capitalist_class != NULL)
{
    // Sum via CLASS equations
    CYCLE(cur, "CLASSES")
        v[0] += VS(cur, "Class_Nominal_Disposable_Income");

    // Sum via direct household aggregation
    v[1] = SUMS(working_class, "Household_Nominal_Disposable_Income") + SUMS(capitalist_class, "Household_Nominal_Disposable_Income");

    // Relative error
    v[2] = (v[1] > 0) ? fabs(v[0] - v[1]) / v[1] : 0;

    if(v[2] > 0.001)
        PLOG("\n[Phase 1] WARNING: Class aggregation error = %.6f (CLASS=%.2f, Direct=%.2f)",
             v[2], v[0], v[1]);
}
else
{
    v[2] = 0;  // Structure not initialized, no error
}

RESULT(v[2])


EQUATION("Test_Class_Share_Sum")
/*
Phase 1: Verify CLASS share equations sum to 1.0.
Checks that working_class + capitalist_class shares = 100%

Returns: Deviation from 1.0 (should be < 0.001)
*/
v[0] = 0;  // error

// Only run if CLASS structure is initialized
if(working_class != NULL && capitalist_class != NULL)
{
    // Income shares
    v[1] = VS(working_class, "Class_Income_Share");
    v[2] = VS(capitalist_class, "Class_Income_Share");
    v[3] = fabs((v[1] + v[2]) - 1.0);

    // Wealth shares
    v[4] = VS(working_class, "Class_Wealth_Share");
    v[5] = VS(capitalist_class, "Class_Wealth_Share");
    v[6] = fabs((v[4] + v[5]) - 1.0);

    // Consumption shares
    v[7] = VS(working_class, "Class_Consumption_Share");
    v[8] = VS(capitalist_class, "Class_Consumption_Share");
    v[9] = fabs((v[7] + v[8]) - 1.0);

    // Max error
    v[0] = max(v[3], max(v[6], v[9]));

    if(v[0] > 0.001)
        PLOG("\n[Phase 1] WARNING: Class shares don't sum to 1.0 (income=%.4f, wealth=%.4f, consumption=%.4f)",
             v[1] + v[2], v[4] + v[5], v[7] + v[8]);
}

RESULT(v[0])

