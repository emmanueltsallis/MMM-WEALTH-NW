/*****GOVERNMENT DECISION VARIABLES*****/

EQUATION("Government_Desired_Wages")
/*
Priority expenses.
If there are no maximum expenses, it is adjusted by average productivity growth and inflation.
*/                                     
	v[0]= LAG_GROWTH(consumption, "Country_Consumer_Price_Index", 1, 1);		   		
	v[1]=V("government_real_wage_growth");
	v[2]=CURRENT*(1+v[0]+v[1]);
RESULT(max(0,v[2]))


EQUATION("Government_Desired_Unemployment_Benefits")
/*
Counter-cyclical Expenses
Benefit is a share of average wage PER WORKER (not wage rate).

Stage 5.5 FIX: Changed from FLOW-based (new layoffs) to STOCK-based (total unemployed).
Original code calculated benefits from sector employment CHANGES, which goes to zero
once layoffs stabilize - leaving a stock of unemployed workers with no benefits.
Now uses Country_Unemployed_Households for proper safety net.

Stage 7 FIX: Uses ACTUAL wage per worker, not wage RATE.
Country_Avg_Nominal_Wage is a wage rate per efficiency unit, not actual income.
Correct formula: Total_Wages / Employed_Workers = actual average wage.

Uses LAGGED values to break circular dependency.
*/
	v[0] = V("government_benefit_rate");
	v[1] = VS(country, "Country_Unemployed_Households");  // Stock of unemployed

	// Calculate actual average wage per employed worker (not wage rate!)
	v[2] = VLS(country, "Country_Total_Wages", 1);        // Lagged total wages
	v[3] = VLS(country, "Country_Employed_Households", 1); // Lagged employed count
	v[4] = (v[3] > 0) ? v[2] / v[3] : 0;                  // Actual avg wage per worker

	// Benefits = unemployed_count × benefit_rate × actual_avg_wage_per_worker
	v[5] = v[1] * v[0] * v[4];

RESULT(max(0, v[5]))


EQUATION("Government_Desired_Investment")
/*
Desired Investment Expenses
Adjusted by a desired real growth rate and avg capital price growth
*/
	v[0]=V("government_real_investment_growth");		
	v[1]=LAG_GROWTH(capital, "Sector_Avg_Price", 1, 1);
	v[2]=CURRENT*(1+v[0]+v[1]);
RESULT(max(0,v[2]))


EQUATION("Government_Desired_Consumption")
/*
Desired Consumption Expenses
Adjusted by a desired real growth rate and avg consumption price growth
*/
	v[0]=V("government_real_consumption_growth");   
	v[1]= LAG_GROWTH(consumption, "Sector_Avg_Price", 1, 1);
	v[2]=CURRENT*(1+v[0]+v[1]);
RESULT(max(0,v[2]))


EQUATION("Government_Desired_Inputs")
/*
Desired Intermediate Expenses
Adjusted by a desired real growth rate and avg input price growth
*/
	v[0]=V("government_real_input_growth");
	v[1]=LAG_GROWTH(input, "Sector_Avg_Price", 1, 1);
	v[2]=CURRENT*(1+v[0]+v[1]);
RESULT(max(0,v[2]))


EQUATION("Government_Desired_Wealth_Transfer")
/*
Stage 7.5: Desired wealth transfer = close-the-gap total from Country level.
Brings households below Xth percentile up to threshold income.
Only active when switch_class_tax_structure >= 5.
*/
v[0] = VS(country, "switch_class_tax_structure");
if(v[0] < 5)
    v[1] = 0;
else
    v[1] = VS(country, "Country_Transfer_Desired");
RESULT(max(0, v[1]))


/*****FISCAL RULES VARIABLES*****/


EQUATION("Government_Surplus_Rate_Target")
/*
Adjusts government surplus target based on debt to gdp evolution
*/
	v[0]=V("annual_frequency");
	v[1]=V("government_max_surplus_target");                     
	v[2]=V("government_min_surplus_target");
	v[3]=LAG_GROWTH(p, "Government_Debt_GDP_Ratio", v[0], 1);
	v[4]=LAG_AVE(p,"Government_Debt_GDP_Ratio",v[0],1);        //annual average debt rate
	v[5]=V("government_max_debt_ratio");                       //maximum debt to gdp accepted, parameter
	v[6]=V("government_min_debt_ratio");                       //minimum debt to gdp accepted, parameter
	v[7]=V("government_surplus_target_adjustment");			   //adjustment parameter
	v[8]=V("begin_flexible_surplus_target");
	
	v[9]= fmod((double) t-1,v[0]);
	if(t>=v[8]&&v[8]!=-1&&v[9]==0)
	{
	if(v[4]>v[5])                           		   								 //if debt to gdp is higher than accepted 
		v[10]=CURRENT+v[7];
	else if (v[4]<v[6])                     		  								 //if debt to gdp is lower than accepted 
		v[10]=CURRENT-v[7];
	else
	{		//if debt to gdp is between acceptable band
		if(v[3]>0)
			v[10]=CURRENT+v[7];
		if(v[3]<0)
			v[10]=CURRENT-v[7];
		else
			v[10]=CURRENT;
	}
	}
	else                                               								 //if flexible surplus target rule is not active
		v[10]=CURRENT;                                    							 //do not change surplus taget  
		
	v[11]=max(min(v[1],v[10]),v[2]);
RESULT(v[11])


EQUATION("Government_Max_Expenses_Surplus")
/*
Government Max Expenses determined by Primary Surplus Target Fiscal rule
*/
	v[0]=VL("Country_GDP",1);
	v[1]=VL("Country_GDP",2);
	v[2]=V("government_expectations");
	v[3]= v[1]!=0? v[2]*(v[0]-v[1])/v[1] : 0;
	v[4]=VL("Government_Total_Taxes",1);
	v[5]=V("Government_Surplus_Rate_Target");
	v[6]=(1+v[3])*(v[4]-v[5]*v[0]);
RESULT(v[6])


EQUATION("Government_Max_Expenses_Ceiling")
/*
Government Max Expenses determined by Expenses Ceiling Target Fiscal rule
*/
	v[1]=LAG_GROWTH(consumption, "Country_Consumer_Price_Index", 1, 1);
	v[2]=VL("Government_Effective_Expenses",1);
	v[3]=v[2]*(1+v[1]);
RESULT(v[3])


EQUATION("Government_Max_Expenses")
/*
Maximum Government expenses imposed by the fiscal rule.
Fiscal rules can be two types: primary surplus target or expenses ceiling (or both).
Depend on the policy parameter.
*/

v[1]=V("begin_surplus_target_rule");                           //define when surplus target rule begins
v[2]=V("begin_expenses_ceiling_rule");                         //define when expenses ceiling begins
																	
v[3]=V("Government_Max_Expenses_Surplus");
v[4]=V("Government_Max_Expenses_Ceiling");

	if ((t>=v[1]&&v[1]!=-1)&&(t>=v[2]&&v[2]!=-1))
		v[5]=min(v[3],v[4]);												//surplus rule and ceiling rule
	else if ((t>=v[1]&&v[1]!=-1)&&(t<v[2]||v[2]==-1))
		v[5]=v[3];															//only surplus rule
	else if ((t<v[1]||v[1]==-1)&&(t>=v[2]&&v[2]!=-1))
		v[5]=v[4];															//only ceiling rule
	else
		v[5]=-1;															//no rule															
RESULT(v[5])


EQUATION("Government_Effective_Expenses")
/*
Stage 5.5: WEIGHTED PROPORTIONAL ALLOCATION
Stage 7.5: Added Wealth Transfer category

When budget is constrained, all spending categories share cuts proportionally
based on vulnerability weights. Lower weight = more protected.

Vulnerability weights (configurable parameters):
- government_cut_weight_wages (default 0.3) - most protected
- government_cut_weight_benefits (default 0.5) - protected
- government_cut_weight_transfer (default 0.6) - Stage 7.5: wealth transfer
- government_cut_weight_consumption (default 1.0) - neutral
- government_cut_weight_inputs (default 1.0) - neutral
- government_cut_weight_investment (default 1.5) - most vulnerable

Example: If 20% cut needed across the board:
- Wages cut: 20% × 0.3 = 6%
- Benefits cut: 20% × 0.5 = 10%
- Transfer cut: 20% × 0.6 = 12%
- Investment cut: 20% × 1.5 = 30%
*/

v[0]=V("Government_Max_Expenses");
v[1]=V("Government_Desired_Wages");
v[2]=V("Government_Desired_Unemployment_Benefits");
v[3]=V("Government_Desired_Consumption");
v[4]=V("Government_Desired_Investment");
v[5]=V("Government_Desired_Inputs");
v[6]=V("Government_Desired_Wealth_Transfer");  // Stage 7.5

if(v[0]==-1)                                               //no fiscal rule
{
	v[8]=v[1];											   //government wages equal desired wages
	v[9]=v[2];    										   //government unemployment benefits equal desired
	v[10]=v[3];                                            //government consumption equal desired
	v[11]=v[5];                                            //government intermediate equal desired
	v[15]=v[4];                                            //government investment demand equals desired
	v[17]=v[6];                                            //government transfer equals desired (Stage 7.5)
}
else  // WITH fiscal rule - WEIGHTED PROPORTIONAL ALLOCATION
{
	// Calculate total desired spending (Stage 7.5: includes transfer)
	v[16] = v[1] + v[2] + v[3] + v[4] + v[5] + v[6];  // Total desired

	if(v[16] <= v[0])  // Budget is sufficient - allocate all desired amounts
	{
		v[8] = v[1];   // wages
		v[9] = v[2];   // benefits
		v[10] = v[3];  // consumption
		v[11] = v[5];  // inputs
		v[15] = v[4];  // investment
		v[17] = v[6];  // transfer (Stage 7.5)
	}
	else  // Budget constrained - apply weighted proportional cuts
	{
		// Get vulnerability weights (lower = more protected)
		v[20] = V("government_cut_weight_wages");
		v[21] = V("government_cut_weight_benefits");
		v[22] = V("government_cut_weight_consumption");
		v[23] = V("government_cut_weight_inputs");
		v[24] = V("government_cut_weight_investment");
		v[28] = V("government_cut_weight_transfer");  // Stage 7.5

		// Defaults if parameters missing or zero
		if(v[20] <= 0) v[20] = 0.3;
		if(v[21] <= 0) v[21] = 0.5;
		if(v[22] <= 0) v[22] = 1.0;
		if(v[23] <= 0) v[23] = 1.0;
		if(v[24] <= 0) v[24] = 1.5;
		if(v[28] <= 0) v[28] = 0.6;  // Stage 7.5: default weight

		// Calculate weighted desired total for normalization
		// Each category's "vulnerability exposure" = desired × weight
		v[25] = v[1]*v[20] + v[2]*v[21] + v[3]*v[22] + v[5]*v[23] + v[4]*v[24] + v[6]*v[28];

		// Total shortfall that needs to be distributed
		v[26] = v[16] - v[0];  // Total shortfall (positive)

		// Distribute shortfall proportionally to weighted amounts
		// Each category's cut = (its_weighted_share / total_weighted) * total_shortfall
		if(v[25] > 0)
		{
			v[8] = v[1] - (v[26] * (v[1]*v[20] / v[25]));   // wages
			v[9] = v[2] - (v[26] * (v[2]*v[21] / v[25]));   // benefits
			v[10] = v[3] - (v[26] * (v[3]*v[22] / v[25]));  // consumption
			v[11] = v[5] - (v[26] * (v[5]*v[23] / v[25]));  // inputs
			v[15] = v[4] - (v[26] * (v[4]*v[24] / v[25]));  // investment
			v[17] = v[6] - (v[26] * (v[6]*v[28] / v[25]));  // transfer (Stage 7.5)
		}
		else  // Fallback: equal cuts if weighted total is 0
		{
			v[27] = v[26] / 6.0;  // Equal cut per category (now 6 categories)
			v[8] = v[1] - v[27];
			v[9] = v[2] - v[27];
			v[10] = v[3] - v[27];
			v[11] = v[5] - v[27];
			v[15] = v[4] - v[27];
			v[17] = v[6] - v[27];  // Stage 7.5
		}

		// Ensure non-negative (can't have negative spending)
		v[8] = max(0, v[8]);
		v[9] = max(0, v[9]);
		v[10] = max(0, v[10]);
		v[11] = max(0, v[11]);
		v[15] = max(0, v[15]);
		v[17] = max(0, v[17]);  // Stage 7.5
	}
}
WRITE("Government_Effective_Wages", max(0,v[8]));
WRITE("Government_Effective_Unemployment_Benefits",  max(0,v[9]));
WRITE("Government_Effective_Consumption",  max(0,v[10]));
WRITE("Government_Effective_Investment",  max(0,v[15]));
WRITE("Government_Effective_Inputs",  max(0,v[11]));
WRITE("Government_Effective_Wealth_Transfer",  max(0,v[17]));  // Stage 7.5
WRITE("Government_Desired_Expenses",  v[1]+v[2]+v[3]+v[4]+v[5]+v[6]);
v[13]=max(0,(v[8]+v[9]+v[10]+v[11]+v[15]+v[17]));
RESULT(v[13])

EQUATION_DUMMY("Government_Effective_Wages","Government_Effective_Expenses")
EQUATION_DUMMY("Government_Effective_Unemployment_Benefits","Government_Effective_Expenses")
EQUATION_DUMMY("Government_Effective_Consumption","Government_Effective_Expenses")
EQUATION_DUMMY("Government_Effective_Investment","Government_Effective_Expenses")
EQUATION_DUMMY("Government_Effective_Inputs","Government_Effective_Expenses")
EQUATION_DUMMY("Government_Effective_Wealth_Transfer","Government_Effective_Expenses")  // Stage 7.5
EQUATION_DUMMY("Government_Desired_Expenses","Government_Effective_Expenses")

/*****GOVERNMENT RESULT VARIABLES*****/


EQUATION("Government_Income_Taxes")
// SWITCHED: From Class_Taxation to Household_Income_Taxation
RESULT(VS(country, "Country_Income_Tax"))

EQUATION("Government_Indirect_Taxes")
RESULT(SUMS(country,"Sector_Taxation"))

EQUATION("Government_Wealth_Tax_Revenue")
/*
Stage 7: Wealth tax revenue collected from households.
Only active when switch_class_tax_structure >= 5.
*/
RESULT(VS(country, "Country_Wealth_Tax_Revenue"))


EQUATION("Government_Penalty_Revenue")
/*
Stage 9: Penalties collected from tax evaders caught by audits.
Only applies to undeclared financial assets (not offshore deposits, which are invisible).
*/
RESULT(VS(country, "Country_Penalty_Revenue"))


EQUATION("Government_Wealth_Tax_Gap")
/*
Stage 9: Revenue lost to tax evasion.
= Tax that WOULD have been owed on hidden wealth.

Tax_Gap = (Offshore_Deposits + Undeclared_Assets) × wealth_tax_rate
Only wealth ABOVE threshold would have been taxed, but this simplified
measure assumes all hidden wealth is above threshold (conservative estimate).
*/
v[0] = VS(country, "Country_Total_Deposits_Offshore");
v[1] = VS(country, "Country_Total_Assets_Undeclared");
v[2] = VS(country, "wealth_tax_rate");
v[3] = (v[0] + v[1]) * v[2];
RESULT(v[3])


EQUATION("Government_Potential_Wealth_Tax_Revenue")
/*
Stage 9: Wealth tax revenue if no evasion occurred.
= Actual collected + Tax gap (lost to evasion)
*/
v[0] = V("Government_Wealth_Tax_Revenue");
v[1] = V("Government_Wealth_Tax_Gap");
RESULT(v[0] + v[1])


EQUATION("Government_Dynamic_Audit_Probability")
/*
Stage 9: Natural decay enforcement model (TODO #3).

Formula:
  p_t = p_base + (1-δ) × (p_{t-1} - p_base) + η × e_{t-1}

Where:
  p_base = audit_probability (minimum enforcement floor)
  δ      = enforcement_decay (capacity depreciation rate)
  η      = enforcement_sensitivity (response to observed evasion)
  e      = Country_Asset_Evasion_Rate (lagged)

Steady state: p* = p_base + (η/δ) × e*

Enforcement capacity builds when evasion is high and decays toward
baseline when evasion subsides. No explicit evasion target needed;
system finds its own equilibrium.

When enforcement_sensitivity = 0, returns base rate (fixed enforcement).
When enforcement_decay = 0, reduces to pure accumulation (no depreciation).
*/
v[0] = V("audit_probability");              // p_base (floor)
v[1] = V("enforcement_sensitivity");        // η (response to evasion)
v[2] = V("enforcement_decay");             // δ (capacity depreciation)

// If no dynamic enforcement, just return base
if(v[1] <= 0)
{
    v[10] = v[0];
}
else
{
    // For first period, use base rate (no history)
    if(t <= 1)
    {
        v[10] = v[0];
    }
    else
    {
        v[3] = VL("Government_Dynamic_Audit_Probability", 1);  // p_{t-1}
        v[4] = VL("Country_Asset_Evasion_Rate", 1);            // e_{t-1}

        // Decay model: base + persistence + response
        v[5] = v[0] + (1.0 - v[2]) * (v[3] - v[0]) + v[1] * v[4];

        // Bound to [0, 1]
        v[10] = max(0.0, min(1.0, v[5]));
    }
}
RESULT(v[10])


EQUATION("Government_Total_Taxes")
/*
Total tax revenue = Income + Indirect + Wealth (Stage 7) + Penalties (Stage 9).
*/
RESULT(V("Government_Income_Taxes") + V("Government_Indirect_Taxes") + V("Government_Wealth_Tax_Revenue") + V("Government_Penalty_Revenue"))

EQUATION("Government_Primary_Result")
RESULT(V("Government_Total_Taxes")-V("Government_Effective_Expenses"))

EQUATION("Government_Interest_Payment")
RESULT(V("Central_Bank_Basic_Interest_Rate")*max(0,VL("Government_Debt",1)))

EQUATION("Government_Nominal_Result")
RESULT(V("Government_Primary_Result")-V("Government_Interest_Payment"))

EQUATION("Government_Debt")
RESULT(CURRENT-V("Government_Nominal_Result")+VS(financial, "Financial_Sector_Rescue"))

EQUATION("Government_Debt_GDP_Ratio")
	v[1]=V("Government_Debt");
	v[2]=V("Country_GDP");
	v[3]= v[2]!=0? v[1]/v[2] : 0;
RESULT(v[3])

EQUATION("Government_Surplus_GDP_Ratio")
	v[1]=V("Government_Primary_Result");
	v[2]=V("Country_GDP");
	v[3]= v[2]!=0? v[1]/v[2] : 0;
RESULT(v[3])

EQUATION("Government_Fiscal_Multiplier")
	v[0]=LAG_GROWTH(p, "Government_Effective_Expenses",V("annual_frequency"));
	v[1]=LAG_GROWTH(country, "Country_GDP",V("annual_frequency"));
	v[2]= v[0]!=0? v[1]/v[0] : 0;
RESULT(v[2])

EQUATION("Government_Investment_GDP_Ratio")
	v[1]=V("Government_Effective_Investment");
	v[2]=V("Country_GDP");
	v[3]= v[2]!=0? v[1]/v[2] : 0;
RESULT(v[3])











