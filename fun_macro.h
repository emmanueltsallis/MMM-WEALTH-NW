/*****MACRO VARIABLES******/


EQUATION("Loans_Distribution_Firms");
/*
Distributed effective loans to firms if there is credit rationing
This variable is very important
It is located in the macro level
There are some underlying hypothesis:
1-Income classes are never credit rationed and receive loans first.
2-A bank has a total amount of loans it can provide. After discounting the amount for the income classes, it distribute proportionally to each sector
3-Within each sector, it provides in a order of debt rate. High indebtedness firms migh not receive loans.
*/

v[0]=SUM("Firm_Demand_Loans");						//total demand of firm loans
v[12]=V("Country_Total_Household_Demand_Loans");				// household loan demand (pre-computed)

CYCLE(cur, "BANKS")
{
	v[1]=VS(cur, "Bank_Effective_Loans");
	v[2]=VS(cur, "bank_id");
	v[13]=VS(cur, "Bank_Market_Share");
	v[14]=v[13]*v[12];
	v[10]=0;
	CYCLES(root, cur1, "SECTORS")
	{
		v[3]=SUMS(cur1, "Firm_Demand_Loans");			//sector demand of loans
		if(v[0]!=0)
			v[4]=v[3]/v[0];								//sector share of demand
		else
			v[4]=0;
		
		v[5]=max(0,(v[1]-v[14])*v[4]);
		v[11]=V("switch_creditworthness");
				
			v[9]=0;
			if(v[11]==1)
				SORTS(root, "FIRMS", "Firm_Avg_Debt_Rate", "UP");
			if(v[11]==2)
				SORTS(root, "FIRMS", "firm_date_birth", "UP");
			if(v[11]==3)
				SORTS(root, "FIRMS", "firm_date_birth", "DOWN");
			CYCLES(cur1, cur2, "FIRMS")
			{
				v[6]=VS(cur2, "firm_bank");
					if (v[6]==v[2])
					{
						v[7]=VS(cur2, "Firm_Demand_Loans");
						if (v[5]>=v[7])
							v[8]=v[7];
						else
							v[8]=max(0, v[5]);
						v[5]=v[5]-v[8];
						WRITES(cur2, "firm_effective_loans", v[8]);
						v[9]=v[9]+1;
					}
					else
					{
						v[5]=v[5];
						v[9]=v[9];
					}		
			}

	v[10]=v[10]+v[9];
	}
WRITES(cur, "Bank_Number_Clients", v[10]);
}	
RESULT(0)

	
EQUATION("Country_Domestic_Intermediate_Demand")
/*
Calculates the domestic demand for inputs.
Must be called by the sectors.
*/
	v[2]=0;                                                      		//initializes the value for thr CYCLE
	CYCLE(cur, "SECTORS")                                        		//CYCLE trought all sectors
		v[2]=v[2]+SUMS(cur, "Firm_Input_Demand_Next_Period");           //sums up the demand for imputs of all setors
	v[0]=VS(input, "Sector_Avg_Price");
	v[1]=V("Government_Effective_Inputs");
	v[5]=v[0]!=0? v[1]/v[0] : 0;
	v[6]=v[2]+v[5];
RESULT(v[6])


EQUATION("Country_Domestic_Consumption_Demand")
/*
Stage 4.7 SWITCH #1: Household-driven consumption demand.
Calculates the domestic demand for consumption goods.
Must be called by the sector.

DEADLOCK FIX: Use SUMS over households directly instead of VS(class,...).
VS(class,...) triggers the Opt 1 master CYCLE which reads ALL household variables,
including any that are currently in-flight. SUMS is safe because
Household_Real_Domestic_Consumption_Demand uses VL(lagged disposable income)
and never depends on current-period Wage_Income or Employment_Status.
*/
	v[0] = SUMS(working_class, "Household_Real_Domestic_Consumption_Demand") + SUMS(capitalist_class, "Household_Real_Domestic_Consumption_Demand");
	v[1]=VS(consumption, "Sector_Avg_Price");
	v[2]=V("Government_Effective_Consumption");
	v[3]= v[1]!=0? v[2]/v[1] : 0;
	v[4]=v[0]+v[3];
RESULT(v[4])


EQUATION("Country_Domestic_Capital_Goods_Demand")
/*
The demand for capital goods is calculated by summing up the demand for capital goods from all sectors with government spending on investment.
Must be called by the sectors.
*/
	v[1]=0;                                                 			//initializes the CYCLE
	CYCLE(cur, "SECTORS")                                   			//CYCLE trought the sectors
	{	
		v[10]=SUMLS(cur, "Firm_Demand_Capital_Goods_Expansion",1);
		v[11]=SUMLS(cur, "Firm_Demand_Capital_Goods_Replacement",1);
		v[1]=v[1]+v[10]+v[11];                                       	//sums up all firm's capital goods demand
	}
	v[4]=VS(capital, "Sector_Avg_Price");
	v[5]=V("Government_Effective_Investment");
	v[6]= v[4]!=0? v[5]/v[4] : 0;
	v[7]=v[1]+v[6];
RESULT(v[7])


EQUATION("Country_Capital_Goods_Price")
RESULT(VS(capital, "Sector_Avg_Price"))


EQUATION("Country_Price_Index")
/*
Average Price of all sector. GDP deflator
*/
	v[0]=WHTAVE("Sector_Sales","Sector_Avg_Price");
	v[1]=SUM("Sector_Sales");
	v[2]= v[1]!=0? v[0]/v[1] : CURRENT;
RESULT(v[2])


EQUATION("Country_Consumer_Price_Index")
/*
Average Price of the consumption goods sector.
Stage 4.7: Uses household import shares weighted by LAGGED income share.
Uses lagged income to avoid circular dependency (same as original Class_Income_Share).

PERFORMANCE OPTIMIZATION: Single-pass computation (was 2 passes, now 1).
Accumulates both total_income and weighted_import_sum simultaneously.
Reduces O(2N) to O(N) - 25% overall household scan reduction.
*/
	v[0]=VS(consumption, "Sector_Avg_Price");
	v[1]=VS(consumption, "Sector_External_Price");
	v[2]=VS(external, "Country_Exchange_Rate");

	// SINGLE PASS: Compute total lagged income AND weighted import sum together
	v[10]=0;  // total_lagged_income = Σ(income_i)
	v[11]=0;  // weighted_import_sum = Σ(import_share_i × income_i)

	CYCLE(cur1, "CLASSES")
	{
	CYCLES(cur1, cur, "HOUSEHOLD")
	{
		v[5]=VS(cur, "Household_Imports_Share");
		v[6]=VLS(cur, "Household_Nominal_Disposable_Income", 1);
		v[10]+=v[6];           // accumulate total income
		v[11]+=v[5]*v[6];      // accumulate weighted import (import_share × income)
	}
	}

	// weighted_avg_import_share = Σ(m_i × y_i) / Σ(y_i)
	v[3]=(v[10] > 0.01) ? v[11]/v[10] : 0;

	v[4]=v[0]*(1-v[3])+v[1]*v[2]*v[3];
RESULT(v[4])



EQUATION("Country_Annual_Inflation")
/*
Annual growth of the overall price index.
Uses support function
*/
RESULT(LAG_GROWTH(p, "Country_Price_Index", V("annual_frequency"), 1))


EQUATION("Country_Annual_CPI_Inflation")
/*
Annual growth of the consumer price index
Uses support function
*/
RESULT(LAG_GROWTH(p, "Country_Consumer_Price_Index", V("annual_frequency"), 1))


EQUATION("Country_Distributed_Profits")
/*
Total amount of distributed profits by the firms. Will be used to determine the income of the income classes.
*/
	v[0]=0;                                            		//initializes the CYCLE
	CYCLE(cur, "SECTORS")                              		//CYCLE trought all sectors
		v[0]=v[0]+SUMS(cur, "Firm_Distributed_Profits");    //sums up the value of distributed profits of all sectors
	v[3]=V("Financial_Sector_Distributed_Profits");
	v[4]=v[0]+v[3];
RESULT(v[4])


EQUATION("Country_Total_Profits")
/*
Total Surplus of the Economy. Is the sum of all firms net profits. Will be used to calculate GDP
*/
	v[0]=0;                                                    		//initializes the CYCLE
	CYCLE(cur, "SECTORS")                                      		//CYCLE trought all sectors
		v[0]=v[0]+SUMS(cur, "Firm_Net_Profits");                    //sums up the surplus of all sectors
	v[3]=V("Financial_Sector_Profits");
	v[4]=v[0]+v[3];
RESULT(v[4])


EQUATION("Country_Total_Wages")
/*
The total wage is calculated by the sum of the wages paid by the sectors with government wages. The wage per unit of production is a predetermined parameter, and the total salary is calculated by multiplying this unit wage by the actual production of each sector.
*/
	v[0]=0;                                                    		//initializes the CYCLE
	CYCLE(cur, "SECTORS")                                      		//CYCLE trought all sectors
	{
		v[1]=0;                                                  	//initializes the second CYCLE
		CYCLES(cur, cur1, "FIRMS")                               	//CYCLE trought all firms of the sector
		{
			v[2]=VS(cur1, "Firm_Wage");                             //firm's wage
			v[3]=VS(cur1, "Firm_Effective_Production");             //firm's effective production
			v[4]=VS(cur1, "Firm_Avg_Productivity");            		//firm's productivity in the last period
			v[5]=VS(cur1, "Firm_RND_Expenses");                     //firm's rnd expeses, returned as salary to researchers		
			if(v[4]!=0)
				v[1]=v[1]+v[3]*(v[2]/v[4])+v[5];               		//sums up all firms' wage, determined by a unitary wage (sectorial wage divided by firm's productivity) multiplied by firm's effective production plus RND expenses
			else
				v[1]=v[1];
		}
		v[0]=v[0]+v[1];                                          	//sums up all wages of all sectors
	}
	v[6]=V("Government_Effective_Wages");                           //wages paid by the government
	v[7]=v[0]+v[6];                                            		//sums up productive sectors wages with government wages
RESULT(v[7])


EQUATION("Country_Avg_Nominal_Wage")
/*
Stage 5.2: Weighted average nominal wage across all sectors.
Used for unemployment benefits calculation.
Weight = sector employment share.
*/
v[0] = VS(consumption, "Sector_Avg_Wage") * VS(consumption, "Sector_Employment");
v[1] = VS(capital, "Sector_Avg_Wage") * VS(capital, "Sector_Employment");
v[2] = VS(input, "Sector_Avg_Wage") * VS(input, "Sector_Employment");
v[3] = VS(consumption, "Sector_Employment") + VS(capital, "Sector_Employment") + VS(input, "Sector_Employment");
v[4] = (v[3] > 0) ? (v[0] + v[1] + v[2]) / v[3] : VS(consumption, "Sector_Avg_Wage");
RESULT(v[4])


EQUATION("Country_Labor_Force")
/*
Stage 5.2: Total labor force (all worker households).
Computed ONCE per period at country level for efficiency.
Households reference this via VS(country, "Country_Labor_Force") - O(1) lookup.
*/
v[0] = COUNTS(working_class, "HOUSEHOLD");  // all worker households (O(1) LSD counter)
RESULT(max(1, v[0]))


EQUATION("Country_Total_Employment")
/*
Stage 5.2: Total employed workers (in any sector).
Computed ONCE per period at country level for efficiency.

DEADLOCK FIX: Use SUMS over Household_Employment_Status directly instead of
VS(class,"Class_Employed_Count"). The master triggered by Class_Employed_Count
reads ALL household variables, including Household_Wage_Income which is in-flight
when this equation is called from within Household_Wage_Income's computation chain.
Household_Employment_Status is safe: it is always already computed before
Household_Wage_Income starts (Employment_Status is higher in the call chain).
Workers have status 0 (unemployed) or 1 (employed); SUM = count of employed workers.
*/
v[0] = SUMS(working_class, "Household_Employment_Status");
RESULT(v[0])


EQUATION("Country_Unemployment_Rate")
/*
Stage 5.2: Unemployment rate = (Labor Force - Employed) / Labor Force
Uses pre-computed country-level aggregates for efficiency.
*/
v[0] = V("Country_Labor_Force");       // Computed once per period
v[1] = V("Country_Total_Employment");  // Computed once per period
v[2] = v[0] - v[1];  // Unemployed
v[3] = (v[0] > 0) ? (v[2] / v[0]) : 0;
RESULT(v[3])


EQUATION("Country_Total_Employed_Skill")
/*
Stage 5.2b: Sum of skill for all employed workers.
Used by Household_Wage_Income for normalized wage distribution.
Ensures identity: SUM(Household_Wage_Income) = Country_Total_Wages
*/
v[0] = 0;
CYCLE(cur1, "CLASSES")
{
CYCLES(cur1, cur, "HOUSEHOLD")
{
    v[1] = VS(cur, "household_type");
    v[2] = VS(cur, "Household_Employment_Status");
    if(v[1] == 0 && v[2] > 0)  // Worker AND employed
    {
        v[3] = VS(cur, "household_skill");
        v[0] += v[3];
    }
}
}
RESULT(max(0.001, v[0]))  // Prevent division by zero


EQUATION("Country_Median_Household_Income")
/*
Median of lagged real household income, used as the transfer eligibility threshold.
Also WRITEs Country_Transfer_Income_Threshold (percentile set by
wealth_transfer_target_percentile) for Country_Transfer_Desired.
Uses nth_element instead of full sort — both values extracted in O(n).
*/
v[0] = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
i = (int) v[0];										// household count

if(i <= 1)
{
    WRITE("Country_Transfer_Income_Threshold", 0);
    v[1] = 0.01;
}
else
{
    static double* inc_vals = nullptr;				// reusable buffer across periods
    static int buf_size = 0;
    if(i > buf_size) { delete[] inc_vals; inc_vals = new double[i * 2]; buf_size = i * 2; }

    j = 0;											// collect lagged incomes (single pass)
    CYCLE(cur1, "CLASSES")
    {
        CYCLES(cur1, cur, "HOUSEHOLD")
            inc_vals[j++] = VLS(cur, "Household_Avg_Real_Income", 1);
    }

    double pos_med = 0.5 * (i - 1);				// median position
    int lo_med = (int)floor(pos_med);
    double frac_med = pos_med - lo_med;
    std::nth_element(inc_vals, inc_vals + lo_med, inc_vals + i);
    double lo_val = inc_vals[lo_med];
    if(frac_med > 1e-10 && lo_med + 1 < i)
    {
        double hi_val = *std::min_element(inc_vals + lo_med + 1, inc_vals + i);
        v[1] = lo_val * (1.0 - frac_med) + hi_val * frac_med;
    }
    else
        v[1] = lo_val;

    v[2] = V("wealth_transfer_target_percentile");	// transfer threshold percentile
    if(v[2] <= 0 || v[2] > 1) v[2] = 0.5;
    double pos_tr = v[2] * (i - 1);
    int lo_tr = (int)floor(pos_tr);
    double frac_tr = pos_tr - lo_tr;
    std::nth_element(inc_vals, inc_vals + lo_tr, inc_vals + i);
    double lo_tr_val = inc_vals[lo_tr];
    if(frac_tr > 1e-10 && lo_tr + 1 < i)
    {
        double hi_tr_val = *std::min_element(inc_vals + lo_tr + 1, inc_vals + i);
        v[3] = lo_tr_val * (1.0 - frac_tr) + hi_tr_val * frac_tr;
    }
    else
        v[3] = lo_tr_val;

    WRITE("Country_Transfer_Income_Threshold", v[3]);
    v[1] = max(0.01, v[1]);
}
RESULT(v[1])


EQUATION("Country_Total_Investment_Expenses")
/*
Aggeregate Investment Expenses is calculated summing up the demand of capital goods of all firms and multiplying by the average price of the capital goods sector
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Effective_Investment_Expenses");
RESULT(v[0])


EQUATION("Country_Profit_Share")
/*
Share of profits over the sum of profits and wages
*/
	v[0]=V("Country_Total_Wages");
	v[1]=V("Country_Total_Profits");
	v[2]=v[0]+v[1];
	v[3]= v[2]!=0? v[1]/v[2] : 0;
RESULT(v[3])


EQUATION("Country_Industrial_Profit_Share")
/*
Share of industrial profits over the sum of profits and wages
*/
	v[0]=0;                                                    		//initializes the CYCLE
	CYCLE(cur, "SECTORS")                                      		//CYCLE trought all sectors
		v[0]=v[0]+SUMS(cur, "Firm_Net_Profits");                    //sums up the surplus of all sectors
	v[1]=V("Country_Total_Wages");
	v[2]=V("Country_Total_Profits");
	v[3]=v[1]+v[2];
	v[4]= v[3]!=0? v[0]/v[3] : 0;
RESULT(v[4])


EQUATION("Country_Financial_Profit_Share")
/*
Share of financial profits over the sum of profits and wages
*/
	v[0]=V("Financial_Sector_Profits");
	v[1]=V("Country_Total_Wages");
	v[2]=V("Country_Total_Profits");
	v[3]=v[1]+v[2];
	v[4]= v[3]!=0? v[0]/v[3] : 0;
RESULT(v[4])


EQUATION("Country_Wage_Share")
/*
Share of profits over the sum of profits and wages
*/
	v[0]=V("Country_Total_Wages");
	v[1]=V("Country_Total_Profits");
	v[2]=v[0]+v[1];
	v[3]= v[2]!=0? v[0]/v[2] : 0;
RESULT(v[3])


EQUATION("Country_Avg_Markup")
/*
Agregated average markup, wheighted by the sales of each sector
*/
	v[0]=WHTAVE("Sector_Avg_Markup", "Sector_Sales");
	v[1]=SUM("Sector_Sales");
	v[2]= v[1]!=0? v[0]/v[1]: 0;
RESULT(v[2])


EQUATION("Country_Debt_Rate_Firms")
/*
Aggregated average debt rate, wheighted by the sales of each sector
*/
	v[0]=WHTAVE("Sector_Avg_Debt_Rate", "Sector_Sales");
	v[1]=SUM("Sector_Sales");
	v[2]= v[1]!=0? v[0]/v[1]: 0;
RESULT(v[2])	


EQUATION("Country_Avg_HHI")
/*
Aggregated average markup, wheighted by the number of firms
*/
	v[0]=WHTAVE("Sector_Normalized_HHI", "Sector_Number_Firms");
	v[1]=SUM("Sector_Number_Firms");
	v[2]= v[1]!=0? v[0]/v[1]: 0;
RESULT(v[2])	


EQUATION("Country_Hedge_Share")
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "firm_hedge");
	v[2]=COUNT_ALL("FIRMS");
	v[3]= v[2]!=0? v[0]/v[2] : 0;
RESULT(v[3])	


EQUATION("Country_Speculative_Share")
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "firm_speculative");
	v[2]=COUNT_ALL("FIRMS");
	v[3]= v[2]!=0? v[0]/v[2] : 0;
RESULT(v[3])


EQUATION("Country_Ponzi_Share")
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "firm_ponzi");
	v[2]=COUNT_ALL("FIRMS");
	v[3]= v[2]!=0? v[0]/v[2] : 0;
RESULT(v[3])


EQUATION("Country_GDP")
/*
Nominal quarterly GDP is calculated summing up profits, wages and indirect taxes
*/
	v[0]=V("Country_Total_Profits");
	v[1]=V("Country_Total_Wages");
	v[2]=V("Government_Indirect_Taxes");
	v[3]=v[0]+v[1]+v[2];
	v[4]=V("Country_GDP_Demand");
	// Diagnostics moved to Diagnostic_Trigger equation
RESULT(v[3])


EQUATION("Diagnostic_Trigger")
/*
Runs after Country_GDP to trigger diagnostic reports.
This avoids circular dependencies by ensuring all macro variables are computed first.
*/
	v[0] = V("Country_GDP");  // Ensure GDP is computed before diagnostics

	// Periodic diagnostic report
	log_diagnostic_report(p, t);

RESULT(1)


EQUATION("Country_Annual_GDP")
RESULT(LAG_SUM(p, "Country_GDP", V("annual_frequency")))


EQUATION("Country_Annual_Real_GDP")
RESULT(LAG_SUM(p, "Country_Real_GDP", V("annual_frequency")))


EQUATION("Country_Real_GDP")
/*
Real quarterly GDP is the nominal GDP over the price index.
*/
	v[0]=V("Country_GDP");              //nominal GDP
	v[1]=V("Country_Price_Index");      //current price index
	v[2]= v[1]!=0? v[0]/v[1] : 0;       //real GDP is the nominal GDP devided by the price index
RESULT(v[2])


EQUATION("Country_Annual_Growth")
/*
Annual Nominal GDP growth rate.
*/
	v[1]=LAG_SUM(p, "Country_GDP", V("annual_frequency"));
	v[2]=LAG_SUM(p, "Country_GDP", V("annual_frequency"), V("annual_frequency") );
	v[3]= v[2]!=0? (v[1]-v[2])/v[2] : 0;
RESULT(v[3])


EQUATION("Country_Annual_Real_Growth")
/*
Annual Real GDP Growth rate.
*/
	v[1]=LAG_SUM(p, "Country_Real_GDP", V("annual_frequency"));
	v[2]=LAG_SUM(p, "Country_Real_GDP", V("annual_frequency"), V("annual_frequency") );
	v[3]= v[2]!=0? (v[1]-v[2])/v[2] : 0;
RESULT(v[3])


EQUATION("Country_Likelihood_Crisis")
/*
Counts the number of crisis ocurrances. 
*/
	v[7]=V("annual_frequency");
	v[0]= fmod((double) t,v[7]);        		//divides the time period by four
	if(v[0]==0)                        		 	//if the rest of the above division is zero (begenning of the year)
		{
		v[1]=V("Country_Annual_Real_Growth");   //real growth rate
		if(v[1]<0)                     			//if the real growth rate is lower the the crisis threshold
			v[3]=1;                         	//counts a crisis
		else                              		//if the real growth rate is not lower the the crisis threshold
			v[3]=0;                         	//do not count a crisis
		}
	else                                		//if the rest of the division is not zero
		v[3]=0;                           		//do not count a crisis   
	v[4]=CURRENT;     							//crisis counter in the last period
	v[5]=v[4]+v[3];                     		//acumulates the crisis counters
	v[6]=(v[5]/t/v[7]);                      	//gives the probability, total crisis counter divided by the number of time periods
RESULT(v[3])


EQUATION("Country_Nominal_Consumption_Production")
RESULT(VS(consumption, "Sector_Sales")*VS(consumption, "Sector_Avg_Price"))

EQUATION("Country_Nominal_Capital_Production")
RESULT(VS(capital, "Sector_Sales")*VS(capital, "Sector_Avg_Price"))

EQUATION("Country_Nominal_Input_Production")
RESULT(VS(input, "Sector_Sales")*VS(input, "Sector_Avg_Price"))

EQUATION("Country_Total_Nominal_Production")
RESULT(WHTAVE("Sector_Avg_Price","Sector_Sales"))

EQUATION("Country_Capacity_Utilization")
/*
Sum up sector's effective production over productive capacity, weighted by sector's nominal value of production over total gross value of production
*/
	v[0]=WHTAVE("Sector_Capacity_Utilization", "Sector_Effective_Production");
	v[1]=SUM("Sector_Effective_Production");
	v[2]= v[1]!=0? v[0]/v[1]: 0;
RESULT(min(1,v[2]))

EQUATION("Country_Idle_Capacity")
RESULT(1-V("Country_Capacity_Utilization"))

EQUATION("Country_Inventories")
RESULT(WHTAVE("Sector_Avg_Price","Sector_Inventories"))


EQUATION("Country_Inventories_Variation")
/*
Sum up the value of changes in iventories of all firms
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Inventories_Variation");
RESULT(v[0])


EQUATION("Country_Avg_Productivity")
/*
Average Productivity of the economy weighted by the employment of each sector
*/
	v[0]=WHTAVE("Sector_Avg_Productivity", "Sector_Employment");
	v[1]=SUM("Sector_Employment");
	v[2]= v[1]!=0? v[0]/v[1] : 0;
RESULT(v[2])


EQUATION("Country_Unemployed_Households")
/*
Stage 5.2: Count of unemployed worker households.
Uses lagged employment state to avoid circular dependencies with income calculations.
Capitalist households (state = -1) are excluded from unemployment count.
*/
v[0] = 0;
CYCLE(cur1, "CLASSES")
{
CYCLES(cur1, cur, "HOUSEHOLD")
{
    v[1] = VLS(cur, "Household_Employment_Status", 1);
    if(v[1] == 0)  // Unemployed worker (state = 0)
        v[0] = v[0] + 1;
}
}
RESULT(v[0])


EQUATION("Country_Employed_Households")
/*
Stage 7: Count of employed worker households.
Uses lagged employment state to avoid circular dependencies.
Needed for calculating actual average wage per worker (for unemployment benefits).
*/
v[0] = 0;
CYCLE(cur1, "CLASSES")
{
CYCLES(cur1, cur, "HOUSEHOLD")
{
    v[1] = VLS(cur, "Household_Employment_Status", 1);
    if(v[1] == 1)  // Employed worker (state = 1)
        v[0] = v[0] + 1;
}
}
RESULT(v[0])


EQUATION("Country_GDP_Demand")
/*
GDP calculated by the demand perspective.
Stage 5.5: Added inventory variation for SFC consistency.
Y = C + I + G + X - M + ΔInventories
*/
	v[0]=V("Country_Total_Household_Expenses");       // C
	v[1]=V("Country_Total_Investment_Expenses");      // I
	v[2]=V("Government_Effective_Expenses");          // G
	v[3]=V("Country_Nominal_Exports");                // X
	v[4]=V("Country_Nominal_Imports");                // M
	v[5]=V("Country_Inventories_Variation");          // ΔInventories (SFC fix)
	v[6]=v[0]+v[1]+v[2]+v[3]-v[4]+v[5];              // Y = C+I+G+X-M+ΔInv
RESULT(v[6])


EQUATION("Country_Real_GDP_Demand")
/*
Real quarterly GDP is the nominal GDP over the price index.
*/
	v[0]=V("Country_GDP_Demand");       //nominal GDP
	v[1]=V("Country_Price_Index");      //current price index
	v[2]= v[1]!=0? v[0]/v[1] : 0;   	//real GDP is the nominal GDP devided by the price index
RESULT(v[2])


EQUATION("Country_Total_Household_Expenses")
/*
Stage 4.7 SWITCH #2: Household expenses.
SWITCHED: From SUM("Class_Effective_Expenses")
          To SUMS(households, "Household_Effective_Expenses")
*/
	v[0] = VS(working_class, "Class_Effective_Expenses") + VS(capitalist_class, "Class_Effective_Expenses");
RESULT(v[0])


/******************************************************************************
 * STAGE 5.3: COUNTRY-LEVEL HOUSEHOLD LOAN AGGREGATES
 * Used by bank equations for cleaner SFC accounting.
 *****************************************************************************/

EQUATION("Country_Total_Household_Stock_Loans")
/*
Stage 5.3: Total household loan stock across all households.
Used by Bank_Stock_Loans_Short_Term for bank accounting.
*/
RESULT(VS(working_class, "Class_Stock_Loans") + VS(capitalist_class, "Class_Stock_Loans"))


EQUATION("Country_Total_Household_Demand_Loans")
/*
Stage 5.3: Total household loan demand across all households.
Used by Bank_Demand_Loans for credit allocation.
*/
RESULT(VS(working_class, "Class_Demand_Loans") + VS(capitalist_class, "Class_Demand_Loans"))


EQUATION("Country_Total_Household_Interest_Payment")
/*
Stage 5.3: Total interest payments from all households.
Used by Bank_Interest_Receivment for bank income.
DEADLOCK FIX: SUMS over households directly. Household_Interest_Payment uses
lagged loan balances so is safe to compute during household income chain.
*/
RESULT(SUMS(working_class, "Household_Interest_Payment") + SUMS(capitalist_class, "Household_Interest_Payment"))


EQUATION("Country_Total_Household_Debt_Payment")
/*
Stage 5.3: Total debt amortization from all households.
Used by Bank_Debt_Payment for loan stock reduction.
*/
RESULT(VS(working_class, "Class_Debt_Payment") + VS(capitalist_class, "Class_Debt_Payment"))


/******************************************************************************
 * STAGE 5.4: FINANCIALIZATION ANALYSIS VARIABLES
 * Track wealth concentration and the decoupling of financial wealth from
 * productive capital (secular stagnation baseline).
 *****************************************************************************/

EQUATION("Country_Total_Financial_Assets")
/*
Stage 5.4: Aggregate financial asset holdings across all households.
Only capitalists hold financial assets (workers have deposits only).
*/
RESULT(VS(working_class, "Class_Financial_Assets") + VS(capitalist_class, "Class_Financial_Assets"))


EQUATION("Country_Total_Household_Stock_Deposits")
/*
Stage 5.4: Aggregate deposit holdings across all households.
Both workers and capitalists hold deposits.
*/
RESULT(VS(working_class, "Class_Stock_Deposits") + VS(capitalist_class, "Class_Stock_Deposits"))


EQUATION("Country_Total_Household_Deposits_Return")
/*
Aggregate interest earned on household deposits.
Used by banking sector for interest payments.
DEADLOCK FIX: SUMS over households directly. Household_Deposits_Return uses
lagged internal funds (VL disposable income) so is safe to compute during
household income chain.
*/
RESULT(SUMS(working_class, "Household_Deposits_Return") + SUMS(capitalist_class, "Household_Deposits_Return"))


EQUATION("Country_Income_Tax")
/*
Aggregate income taxation collected from all households.
Used by government for income tax collection.
*/
RESULT(VS(working_class, "Class_Income_Taxation") + VS(capitalist_class, "Class_Income_Taxation"))


EQUATION("Country_Wealth_Tax_Revenue")
/*
Stage 7: Aggregate wealth tax collected from all households.
Only active when switch_class_tax_structure >= 5.
*/
v[0] = V("switch_class_tax_structure");
if(v[0] < 5)
    v[1] = 0;
else
    v[1] = VS(working_class, "Class_Wealth_Tax_Payment") + VS(capitalist_class, "Class_Wealth_Tax_Payment");
RESULT(v[1])


EQUATION("Country_Wealth_Tax_Taxpayer_Count")
/*
Stage 7: Count of households paying wealth tax.
Useful for distributional analysis.
*/
v[0] = V("switch_class_tax_structure");
if(v[0] < 5)
{
    v[1] = 0;
}
else
{
    v[1] = 0;
    CYCLE(cur1, "CLASSES")
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        v[2] = VS(cur, "Household_Wealth_Tax_Payment");
        if(v[2] > 0.01)
            v[1]++;
    }
    }
}
RESULT(v[1])


EQUATION("Country_Wealth_Tax_From_Deposits")
/*
Stage 7: Aggregate wealth tax paid from deposits.
*/
v[0] = V("switch_class_tax_structure");
if(v[0] < 5)
    v[1] = 0;
else
    v[1] = VS(working_class, "Class_Wealth_Tax_From_Deposits") + VS(capitalist_class, "Class_Wealth_Tax_From_Deposits");
RESULT(v[1])


EQUATION("Country_Wealth_Tax_From_Assets")
/*
Stage 7: Aggregate wealth tax paid via asset liquidation.
*/
v[0] = V("switch_class_tax_structure");
if(v[0] < 5)
    v[1] = 0;
else
    v[1] = VS(working_class, "Class_Wealth_Tax_From_Assets") + VS(capitalist_class, "Class_Wealth_Tax_From_Assets");
RESULT(v[1])


EQUATION("Country_Wealth_Tax_From_Borrowing")
/*
Stage 7: Aggregate wealth tax paid via borrowing.
*/
v[0] = V("switch_class_tax_structure");
if(v[0] < 5)
    v[1] = 0;
else
    v[1] = VS(working_class, "Class_Wealth_Tax_From_Borrowing") + VS(capitalist_class, "Class_Wealth_Tax_From_Borrowing");
RESULT(v[1])


EQUATION("Country_Wealth_Tax_From_Buffer")
/*
Stage 7: Aggregate wealth tax paid by invading liquidity buffer (Stage 4).
Nonzero indicates widespread household financial distress.
*/
v[0] = V("switch_class_tax_structure");
if(v[0] < 5)
    v[1] = 0;
else
    v[1] = VS(working_class, "Class_Wealth_Tax_From_Buffer") + VS(capitalist_class, "Class_Wealth_Tax_From_Buffer");
RESULT(v[1])


EQUATION("Country_Wealth_Capital_Ratio")
/*
Stage 5.4: VALUATION RATIO (Kaldor's v / Tobin's q) = Total Wealth / Productive Capital.
v > 1 indicates financialization (paper wealth exceeds real capital).

Watch this grow as secular stagnation progresses!
- v ≈ 1.0: Balanced economy (wealth backed by real capital)
- v > 1.5: Financialization (paper wealth decoupled from production)
- v > 2.0: Significant asset bubble territory
*/
v[0] = V("Country_Total_Financial_Assets");					// paper wealth (pre-computed)
v[1] = V("Country_Total_Household_Stock_Deposits");			// liquid wealth (pre-computed)
v[2] = V("Country_Capital_Stock");
v[3] = (v[2] > 0) ? ((v[0] + v[1]) / v[2]) : 1.0;
RESULT(v[3])


EQUATION("Country_Wealth_GDP_Ratio")
/*
Stage 5.4: Piketty's β = Net Wealth / GDP.
How many years of GDP equals total household wealth.
Typical values: β = 4-6 in developed economies.
Uses lagged GDP to avoid circular dependency with diagnostics.
*/
v[0] = V("Country_Total_Household_Net_Wealth");
v[1] = VL("Country_GDP", 1);  // Lagged to avoid deadlock (diagnostic called from Country_GDP)
v[2] = (v[1] > 0) ? (v[0] / v[1]) : 0;
RESULT(v[2])


EQUATION("Country_Capitalist_Wealth_Share")
/*
Stage 5.4: Fraction of total household wealth held by capitalists.
Should approach 1.0 as financialization progresses (wealth concentration).

Total Wealth = Financial Assets + Deposits - Loans (for each household)
*/
v[0] = 0;  // Capitalist wealth
v[1] = 0;  // Worker wealth
CYCLE(cur1, "CLASSES")
{
CYCLES(cur1, cur, "HOUSEHOLD")
{
    v[2] = VS(cur, "household_type");
    v[3] = VS(cur, "Household_Financial_Assets");
    v[4] = VS(cur, "Household_Stock_Deposits");
    v[5] = VS(cur, "Household_Stock_Loans");
    v[6] = v[3] + v[4] - v[5];  // Net wealth

    if(v[2] == 1)
        v[0] += v[6];  // Capitalist
    else
        v[1] += v[6];  // Worker
}
}
v[7] = v[0] + v[1];
v[8] = (v[7] > 0) ? (v[0] / v[7]) : 0;
RESULT(v[8])


EQUATION("Country_Total_Household_Net_Wealth")
/*
Stage 5.4: Total net wealth across all households.
Net Wealth = Deposits + Financial Assets - Loans
*/
RESULT(VS(working_class, "Class_Net_Wealth") + VS(capitalist_class, "Class_Net_Wealth"))


/******************************************************************************
 * STAGE 9: TAX EVASION & CAPITAL FLIGHT AGGREGATIONS
 *****************************************************************************/


EQUATION("Country_Total_Deposits_Offshore")
/*
Stage 9: Total deposits held in offshore tax havens.
Invisible to government, not subject to wealth tax.
*/
RESULT(VS(working_class, "Class_Deposits_Offshore") + VS(capitalist_class, "Class_Deposits_Offshore"))


EQUATION("Country_Total_Assets_Undeclared")
/*
Stage 9: Total financial assets not declared to tax authority.
Subject to audit risk and penalties if caught.
*/
RESULT(VS(working_class, "Class_Assets_Undeclared") + VS(capitalist_class, "Class_Assets_Undeclared"))


EQUATION("Country_Total_Assets_Declared")
/*
Stage 9: Total financial assets declared to tax authority.
*/
RESULT(VS(working_class, "Class_Assets_Declared") + VS(capitalist_class, "Class_Assets_Declared"))


EQUATION("Country_Capital_Flight_Rate")
/*
Stage 9: Fraction of total deposits held offshore.
= Offshore_Deposits / Total_Deposits
*/
v[0] = V("Country_Total_Deposits_Offshore");
v[1] = V("Country_Total_Household_Stock_Deposits");
v[2] = (v[1] > 0.01) ? v[0] / v[1] : 0;
RESULT(v[2])


EQUATION("Country_Asset_Evasion_Rate")
/*
Stage 9: Fraction of financial assets undeclared.
= Undeclared_Assets / Total_Financial_Assets
*/
v[0] = V("Country_Total_Assets_Undeclared");
v[1] = V("Country_Total_Financial_Assets");					// total financial assets (pre-computed)
v[2] = (v[1] > 0.01) ? v[0] / v[1] : 0;
RESULT(v[2])


EQUATION("Country_Evader_Count")
/*
Stage 9: Number of households with hidden wealth.
= Offshore_Deposits > 0 OR Undeclared_Assets > 0
*/
v[0] = 0;
CYCLE(cur1, "CLASSES")
{
CYCLES(cur1, cur, "HOUSEHOLD")
{
    v[1] = VS(cur, "Household_Deposits_Offshore");
    v[2] = VS(cur, "Household_Assets_Undeclared");
    if(v[1] > 0.01 || v[2] > 0.01)
        v[0] += 1;
}
}
RESULT(v[0])


EQUATION("Country_Audit_Count")
/*
Stage 9: Number of households audited this period.
*/
RESULT(VS(working_class, "Class_Audited_Count") + VS(capitalist_class, "Class_Audited_Count"))


EQUATION("Country_Penalty_Revenue")
/*
Stage 9: Total penalties collected from caught evaders.
Includes both asset evasion penalties and offshore deposit penalties.
Goes to Government_Penalty_Revenue.
*/
v[0] = VS(working_class, "Class_Asset_Penalty") + VS(capitalist_class, "Class_Asset_Penalty");
v[1] = VS(working_class, "Class_Offshore_Penalty") + VS(capitalist_class, "Class_Offshore_Penalty");
RESULT(v[0] + v[1])


// Country_Tax_Gap moved to Government_Wealth_Tax_Gap in fun_government.h (Stage 9)


EQUATION("Country_Productive_Capacity_Depreciated")
/*
Sum up the value of depreciated productive capacity of all firms
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Productive_Capacity_Depreciation");
RESULT(v[0])


EQUATION("Country_Productive_Capacity_Expansion")
/*
Sum up the value of productive capacity for expanstion of all firms
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Demand_Productive_Capacity_Expansion");
RESULT(v[0])


EQUATION("Country_Productive_Capacity_Replacement")
/*
Sum up the value of productive capacity for replacement of all firms
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Demand_Productive_Capacity_Replacement");
RESULT(v[0])


EQUATION("Country_Capital_Stock")
/*
Sum up the nominal value of firms stock of capital
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Capital");
RESULT(v[0])


EQUATION("Country_Capital_Output_Ratio")
/*
Observed Ratio, Stock of Capital over GDP
*/
	v[0]=V("Country_GDP");
	v[1]=V("Country_Capital_Stock");
	v[2]= v[0]!=0? v[1]/v[0] : 0;
RESULT(v[2])


EQUATION("Country_Capital_Labor_Ratio")
/*
Observed Ratio, Stock of Capital over Total Employment
*/
	v[0]=SUM("Sector_Employment");
	v[1]=V("Country_Capital_Stock");
	v[2]= v[0]!=0? v[1]/v[0] : 0;
RESULT(v[2])


EQUATION("Country_Avg_Profit_Rate")
/*
Observed Ratio, Total Profits over Stock of Capital
*/
	v[0]=V("Country_Total_Profits")-V("Financial_Sector_Profits");
	v[1]=V("Country_Capital_Stock");
	v[2]= v[1]!=0? v[0]/v[1] : 0;
RESULT(v[2])


EQUATION("Country_Induced_Investment")
/*
Sum up the nominal value of effective expansion investment of all firms
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Effective_Expansion_Investment_Expenses");
RESULT(v[0])


EQUATION("Country_Autonomous_Investment")
/*
Sum up the nominal value of effective replacement investment of all firms
*/
	v[0]=0;
	CYCLE(cur, "SECTORS")
		v[0]=v[0]+SUMS(cur, "Firm_Replacement_Expenses");
RESULT(v[0])


EQUATION("Country_Autonomous_Consumption")
/*
Stage 4.7 SWITCH #3: Household autonomous consumption.
Sum up nominal value of autonomous consumption.
SWITCHED: From SUM("Class_Real_Autonomous_Consumption")
          To SUMS(households, "Household_Real_Autonomous_Consumption")
*/
	v[0]=VS(working_class, "Class_Real_Autonomous_Consumption") + VS(capitalist_class, "Class_Real_Autonomous_Consumption");
	v[1]=VS(consumption, "Sector_Avg_Price");
	v[2]=v[0]*v[1];
RESULT(v[2])


EQUATION("Country_Debt_Rate")
/*
Aggregated average household debt rate, weighted by income.
Debt Rate = Stock_Loans / Stock_Deposits for each household.
*/
	v[10] = 0;  // weighted sum of debt rates
	v[11] = 0;  // sum of incomes (weights)
	CYCLE(cur1, "CLASSES")
	{
	CYCLES(cur1, cur, "HOUSEHOLD")
	{
		v[0] = VS(cur, "Household_Stock_Deposits");
		v[1] = VS(cur, "Household_Stock_Loans");
		v[2] = VS(cur, "Household_Nominal_Disposable_Income");
		v[3] = (v[0] > 0.001) ? v[1] / v[0] : 0;  // debt rate = loans/deposits
		v[10] += v[3] * v[2];  // weighted by income
		v[11] += v[2];
	}
	}
	v[12] = (v[11] > 0.001) ? v[10] / v[11] : 0;
RESULT(v[12])


/******************************************************************************
 * INEQUALITY INDICES
 *
 * Master equations collect household data once per annual period, sort once,
 * and compute all indices (Gini, Palma, Top10, Top1, Bottom50, Theil) in a
 * single pass. Slave variables are EQUATION_DUMMY, filled via WRITE.
 ******************************************************************************/

EQUATION("Country_Gini_Index")
/*
Master equation for post-tax income inequality (disposable income).
Computes Gini, Palma, Top10, Top1, Bottom50 and Theil in a single sorted pass.
Recomputes only at annual frequency; holds previous value in interim periods.
*/
	v[0] = V("annual_frequency");						// annual recomputation period
	v[1] = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
	v[2] = 0;											// result (gini)

	if(t > 1 && fmod((double) t, v[0]) != 0)
		v[2] = CURRENT;									// hold previous value
	else if(v[1] <= 1)
	{
		WRITE("Country_Palma_Ratio_Income", 0);
		WRITE("Country_Top10_Share_Income", 0);
		WRITE("Country_Top1_Share_Income", 0);
		WRITE("Country_Bottom50_Share_Income", 0);
		WRITE("Country_Theil_Index_Income", 0);
	}
	else
	{
		i = (int) v[1];
		static double* values = nullptr;				// reusable buffer across periods
		static int buf_size = 0;
		if(i > buf_size) { delete[] values; values = new double[i * 2]; buf_size = i * 2; }

		double total = 0;
		j = 0;
		CYCLE(cur1, "CLASSES")
		{
		CYCLES(cur1, cur, "HOUSEHOLD")
		{
			values[j] = VS(cur, "Household_Nominal_Disposable_Income");
			total += values[j];
			j++;
		}
		}

		double mean = total / i;						// Theil computed before sorting
		double theil = 0;
		for(int k = 0; k < i; k++) {
			double ratio = max(1e-10, values[k]) / mean;
			if(ratio > 1e-10) theil += ratio * log(ratio);
		}
		WRITE("Country_Theil_Index_Income", theil / i);

		std::sort(values, values + i);

		double sum_ix = 0, sum_x = 0;
		for(int k = 0; k < i; k++) {
			sum_ix += (k + 1) * values[k];
			sum_x  += values[k];
		}
		v[2] = (sum_x > 1e-10) ? (2.0 * sum_ix - (i + 1) * sum_x) / (i * sum_x) : 0;

		int b40 = (int)(i * 0.40), b50 = (int)(i * 0.50);
		int t10 = (int)(i * 0.90), t1  = min((int)(i * 0.99), i - 1);
		double s_b40 = 0, s_b50 = 0, s_t10 = 0, s_t1 = 0;
		for(int k = 0; k < i; k++) {
			if(k < b40) s_b40 += values[k];
			if(k < b50) s_b50 += values[k];
			if(k >= t10) s_t10 += values[k];
			if(k >= t1)  s_t1  += values[k];
		}
		WRITE("Country_Palma_Ratio_Income",   (fabs(s_b40) > 1e-10) ? s_t10 / s_b40 : 9999);
		WRITE("Country_Top10_Share_Income",   (total > 1e-10) ? s_t10 / total : 0);
		WRITE("Country_Top1_Share_Income",    (total > 1e-10) ? s_t1  / total : 0);
		WRITE("Country_Bottom50_Share_Income",(total > 1e-10) ? s_b50 / total : 0);
	}
RESULT(v[2])

EQUATION("Country_Gini_Index_Pretax")
/*
Gini coefficient for gross (pre-tax) income distribution.
Under proportional income tax (switch_class_tax_structure < 5), the Gini is
scale-invariant and equals Country_Gini_Index — no sort required.
Recomputes only at annual frequency; holds previous value in interim periods.
*/
	v[0] = V("annual_frequency");						// annual recomputation period
	v[1] = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
	v[2] = 0;											// result (gini)

	if(t > 1 && fmod((double) t, v[0]) != 0)
		v[2] = CURRENT;									// hold previous value
	else if(v[1] <= 1)
		v[2] = 0;
	else
	{
		v[3] = V("switch_class_tax_structure");
		if(v[3] < 5)
			v[2] = V("Country_Gini_Index");				// proportional tax: Gini is scale-invariant
		else
		{
			i = (int) v[1];
			static double* values = nullptr;
			static int buf_size = 0;
			if(i > buf_size) { delete[] values; values = new double[i * 2]; buf_size = i * 2; }

			j = 0;
			CYCLE(cur1, "CLASSES")
			{
			CYCLES(cur1, cur, "HOUSEHOLD")
			{
				values[j] = VS(cur, "Household_Nominal_Gross_Income");
				j++;
			}
			}

			std::sort(values, values + i);
			double sum_ix = 0, sum_x = 0;
			for(int k = 0; k < i; k++) {
				sum_ix += (k + 1) * values[k];
				sum_x  += values[k];
			}
			v[2] = (sum_x > 1e-10) ? (2.0 * sum_ix - (i + 1) * sum_x) / (i * sum_x) : 0;
		}
	}
RESULT(v[2])

EQUATION("Country_Gini_Index_Wealth")
/*
Master equation for post-tax wealth inequality (net wealth).
Computes Gini, Palma, Top10, Top1, Bottom50 and Theil in a single sorted pass.
Recomputes only at annual frequency; holds previous value in interim periods.
*/
	v[0] = V("annual_frequency");						// annual recomputation period
	v[1] = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
	v[2] = 0;											// result (gini)

	if(t > 1 && fmod((double) t, v[0]) != 0)
		v[2] = CURRENT;									// hold previous value
	else if(v[1] <= 1)
	{
		WRITE("Country_Palma_Ratio_Wealth", 0);
		WRITE("Country_Top10_Share_Wealth", 0);
		WRITE("Country_Top1_Share_Wealth", 0);
		WRITE("Country_Bottom50_Share_Wealth", 0);
		WRITE("Country_Theil_Index_Wealth", 0);
	}
	else
	{
		i = (int) v[1];
		static double* values = nullptr;				// reusable buffer across periods
		static int buf_size = 0;
		if(i > buf_size) { delete[] values; values = new double[i * 2]; buf_size = i * 2; }

		double total = 0;
		j = 0;
		CYCLE(cur1, "CLASSES")
		{
		CYCLES(cur1, cur, "HOUSEHOLD")
		{
			values[j] = VS(cur, "Household_Net_Wealth");
			total += values[j];
			j++;
		}
		}

		double mean = total / i;						// Theil computed before sorting
		double theil = 0;
		for(int k = 0; k < i; k++) {
			double ratio = max(1e-10, values[k]) / mean;
			if(ratio > 1e-10) theil += ratio * log(ratio);
		}
		WRITE("Country_Theil_Index_Wealth", theil / i);

		std::sort(values, values + i);

		double sum_ix = 0, sum_x = 0;
		for(int k = 0; k < i; k++) {
			sum_ix += (k + 1) * values[k];
			sum_x  += values[k];
		}
		v[2] = (sum_x > 1e-10) ? (2.0 * sum_ix - (i + 1) * sum_x) / (i * sum_x) : 0;

		int b40 = (int)(i * 0.40), b50 = (int)(i * 0.50);
		int t10 = (int)(i * 0.90), t1  = min((int)(i * 0.99), i - 1);
		double s_b40 = 0, s_b50 = 0, s_t10 = 0, s_t1 = 0;
		for(int k = 0; k < i; k++) {
			if(k < b40) s_b40 += values[k];
			if(k < b50) s_b50 += values[k];
			if(k >= t10) s_t10 += values[k];
			if(k >= t1)  s_t1  += values[k];
		}
		WRITE("Country_Palma_Ratio_Wealth",   (fabs(s_b40) > 1e-10) ? s_t10 / s_b40 : 9999);
		WRITE("Country_Top10_Share_Wealth",   (total > 1e-10) ? s_t10 / total : 0);
		WRITE("Country_Top1_Share_Wealth",    (total > 1e-10) ? s_t1  / total : 0);
		WRITE("Country_Bottom50_Share_Wealth",(total > 1e-10) ? s_b50 / total : 0);
	}
RESULT(v[2])

EQUATION("Country_Gini_Index_Wealth_Pretax")
/*
Master equation for pre-tax wealth inequality (net wealth + wealth tax paid).
When wealth_tax_rate = 0, pre-tax and post-tax wealth are identical, so all
indices are copied from Country_Gini_Index_Wealth without iterating households.
Recomputes only at annual frequency; holds previous value in interim periods.
*/
	v[0] = V("annual_frequency");						// annual recomputation period
	v[1] = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
	v[2] = 0;											// result (gini)

	if(t > 1 && fmod((double) t, v[0]) != 0)
		v[2] = CURRENT;									// hold previous value
	else if(v[1] <= 1)
	{
		WRITE("Country_Palma_Ratio_Wealth_Pretax", 0);
		WRITE("Country_Top10_Share_Wealth_Pretax", 0);
		WRITE("Country_Top1_Share_Wealth_Pretax", 0);
		WRITE("Country_Bottom50_Share_Wealth_Pretax", 0);
		WRITE("Country_Theil_Index_Wealth_Pretax", 0);
	}
	else
	{
		v[3] = V("wealth_tax_rate");
		if(v[3] <= 0)									// no tax: pre-tax = post-tax
		{
			WRITE("Country_Palma_Ratio_Wealth_Pretax",    V("Country_Palma_Ratio_Wealth"));
			WRITE("Country_Top10_Share_Wealth_Pretax",    V("Country_Top10_Share_Wealth"));
			WRITE("Country_Top1_Share_Wealth_Pretax",     V("Country_Top1_Share_Wealth"));
			WRITE("Country_Bottom50_Share_Wealth_Pretax", V("Country_Bottom50_Share_Wealth"));
			WRITE("Country_Theil_Index_Wealth_Pretax",    V("Country_Theil_Index_Wealth"));
			v[2] = V("Country_Gini_Index_Wealth");
		}
		else
		{
			i = (int) v[1];
			static double* values = nullptr;			// reusable buffer across periods
			static int buf_size = 0;
			if(i > buf_size) { delete[] values; values = new double[i * 2]; buf_size = i * 2; }

			double total = 0;
			j = 0;
			CYCLE(cur1, "CLASSES")
			{
			CYCLES(cur1, cur, "HOUSEHOLD")
			{
				values[j] = VS(cur, "Household_Net_Wealth") + VS(cur, "Household_Wealth_Tax_Payment");
				total += values[j];
				j++;
			}
			}

			double mean = total / i;					// Theil computed before sorting
			double theil = 0;
			for(int k = 0; k < i; k++) {
				double ratio = max(1e-10, values[k]) / mean;
				if(ratio > 1e-10) theil += ratio * log(ratio);
			}
			WRITE("Country_Theil_Index_Wealth_Pretax", theil / i);

			std::sort(values, values + i);

			double sum_ix = 0, sum_x = 0;
			for(int k = 0; k < i; k++) {
				sum_ix += (k + 1) * values[k];
				sum_x  += values[k];
			}
			v[2] = (sum_x > 1e-10) ? (2.0 * sum_ix - (i + 1) * sum_x) / (i * sum_x) : 0;

			int b40 = (int)(i * 0.40), b50 = (int)(i * 0.50);
			int t10 = (int)(i * 0.90), t1  = min((int)(i * 0.99), i - 1);
			double s_b40 = 0, s_b50 = 0, s_t10 = 0, s_t1 = 0;
			for(int k = 0; k < i; k++) {
				if(k < b40) s_b40 += values[k];
				if(k < b50) s_b50 += values[k];
				if(k >= t10) s_t10 += values[k];
				if(k >= t1)  s_t1  += values[k];
			}
			WRITE("Country_Palma_Ratio_Wealth_Pretax",   (fabs(s_b40) > 1e-10) ? s_t10 / s_b40 : 9999);
			WRITE("Country_Top10_Share_Wealth_Pretax",   (total > 1e-10) ? s_t10 / total : 0);
			WRITE("Country_Top1_Share_Wealth_Pretax",    (total > 1e-10) ? s_t1  / total : 0);
			WRITE("Country_Bottom50_Share_Wealth_Pretax",(total > 1e-10) ? s_b50 / total : 0);
		}
	}
RESULT(v[2])


/******************************************************************************
 * INEQUALITY INDICES: Dummy Variables (Income)
 * These are computed by Country_Gini_Index via WRITE()
 ******************************************************************************/

EQUATION_DUMMY("Country_Palma_Ratio_Income", "Country_Gini_Index")
EQUATION_DUMMY("Country_Top10_Share_Income", "Country_Gini_Index")
EQUATION_DUMMY("Country_Top1_Share_Income", "Country_Gini_Index")
EQUATION_DUMMY("Country_Bottom50_Share_Income", "Country_Gini_Index")
EQUATION_DUMMY("Country_Theil_Index_Income", "Country_Gini_Index")


/******************************************************************************
 * INEQUALITY INDICES: Dummy Variables (Wealth, post-tax)
 * These are computed by Country_Gini_Index_Wealth via WRITE()
 ******************************************************************************/

EQUATION_DUMMY("Country_Palma_Ratio_Wealth", "Country_Gini_Index_Wealth")
EQUATION_DUMMY("Country_Top10_Share_Wealth", "Country_Gini_Index_Wealth")
EQUATION_DUMMY("Country_Top1_Share_Wealth", "Country_Gini_Index_Wealth")
EQUATION_DUMMY("Country_Bottom50_Share_Wealth", "Country_Gini_Index_Wealth")
EQUATION_DUMMY("Country_Theil_Index_Wealth", "Country_Gini_Index_Wealth")


/******************************************************************************
 * INEQUALITY INDICES: Dummy Variables (Wealth, pre-tax)
 * These are computed by Country_Gini_Index_Wealth_Pretax via WRITE()
 ******************************************************************************/

EQUATION_DUMMY("Country_Palma_Ratio_Wealth_Pretax", "Country_Gini_Index_Wealth_Pretax")
EQUATION_DUMMY("Country_Top10_Share_Wealth_Pretax", "Country_Gini_Index_Wealth_Pretax")
EQUATION_DUMMY("Country_Top1_Share_Wealth_Pretax", "Country_Gini_Index_Wealth_Pretax")
EQUATION_DUMMY("Country_Bottom50_Share_Wealth_Pretax", "Country_Gini_Index_Wealth_Pretax")
EQUATION_DUMMY("Country_Theil_Index_Wealth_Pretax", "Country_Gini_Index_Wealth_Pretax")


EQUATION("Country_Avg_Propensity_Consume")
/*
Stage 4.7 SWITCH #4: Household-based average propensity.
SWITCHED: From SUM("Class_*") To SUMS(households, "Household_*")
*/
	v[0]=VS(consumption, "Sector_Avg_Price");
	v[1]=VS(working_class, "Class_Effective_Real_Domestic_Consumption") + VS(capitalist_class, "Class_Effective_Real_Domestic_Consumption");
	v[2]=VS(working_class, "Class_Nominal_Disposable_Income") + VS(capitalist_class, "Class_Nominal_Disposable_Income");
	v[3]= v[2]!=0? v[0]*v[1]/v[2] : 0;
RESULT(v[3])


/******************************************************************************
 * STAGE 7.5: WEALTH TRANSFER (Equal Distribution to Bottom X%)
 *
 * OPTIMIZED POLICY:
 * - Eligibility: Bottom X% by income (parameter: wealth_transfer_target_percentile)
 * - Distribution: Equal transfer to all eligible households
 * - Budget: Wealth tax revenue (goes through government budget allocation)
 * - OPTIMIZATION: Eligibility flag written during count (no double check)
 *
 * Flow: Wealth_Tax → Country_Transfer_Desired → Gov_Budget → Equal split
 *
 * REQUIRED:
 * - wealth_transfer_target_percentile (COUNTRY level parameter, default 0.5)
 * - Household_Transfer_Eligible (HOUSEHOLD level EQUATION_DUMMY, written here)
 *****************************************************************************/


EQUATION("Country_Transfer_Desired")
/*
Stage 7.5 OPTIMIZED: Equal transfer to bottom X% of households.

BUDGET SOURCE: Wealth tax revenue (goes through government budget allocation).
ELIGIBILITY: Income <= threshold (PERCLS with wealth_transfer_target_percentile).
OPTIMIZATION: Marks each household's eligibility via WRITES during the count loop,
              so Household_Transfer_Received doesn't need to recompute.

Parameter: wealth_transfer_target_percentile (default 0.5 = median = bottom 50%)

Flow: Wealth_Tax → Desired → Gov_Budget → Effective → Equal split to households
*/
v[0] = VS(country, "switch_class_tax_structure");
if(v[0] < 5)
{
    // Clear all eligibility flags
    CYCLE(cur1, "CLASSES")
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        WRITES(cur, "Household_Transfer_Eligible", 0);
    }
    }
    WRITE("Country_Transfer_Eligible", 0);
    v[10] = 0;
}
else
{
    // Read pre-computed threshold (sorted once in Country_Median_Household_Income)
    V("Country_Median_Household_Income");  // Ensure master equation runs first
    v[2] = V("Country_Transfer_Income_Threshold");

    // Count eligible AND mark each household in ONE pass
    v[3] = 0;  // Eligible count
    CYCLE(cur1, "CLASSES")
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        v[4] = VLS(cur, "Household_Avg_Real_Income", 1);
        if(v[4] <= v[2])  // Income at or below threshold
        {
            WRITES(cur, "Household_Transfer_Eligible", 1);
            v[3]++;
        }
        else
        {
            WRITES(cur, "Household_Transfer_Eligible", 0);
        }
    }
    }

    WRITE("Country_Transfer_Eligible", v[3]);

    // BUDGET = Wealth tax revenue (lagged: government budgets transfers from last period's actual receipts)
    v[10] = VL("Country_Wealth_Tax_Revenue", 1);
}
RESULT(max(0, v[10]))


EQUATION("Country_Transfer_Eligible")
/*
Stage 7.5: Count of households eligible for wealth transfer (bottom X%).
Value is set via WRITE in Country_Transfer_Desired.
*/
RESULT(CURRENT)

EQUATION_DUMMY("Country_Transfer_Income_Threshold", "Country_Median_Household_Income")

