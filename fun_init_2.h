
EQUATION("Initialization_2")

consumption=SEARCH_CND("id_consumption_goods_sector",1);
capital=SEARCH_CND("id_capital_goods_sector",1);
input=SEARCH_CND("id_intermediate_goods_sector",1);
government=SEARCH("GOVERNMENT");
financial=SEARCH("FINANCIAL");
external=SEARCH("EXTERNAL_SECTOR");
country=SEARCH("COUNTRY");
centralbank=SEARCH("CENTRAL_BANK");

// NOTE: aclass, bclass, cclass searches removed (Stage 5.5 CLASS removal)
// All consumption now driven by HOUSEHOLD objects

// Phase 1: Initialize household pointers
// Structure: COUNTRY → CLASSES (workers/capitalists) → HOUSEHOLD
// (Mirrors: COUNTRY → SECTORS → FIRMS)
//
// Pointers:
//   households       → country (common ancestor, for SUMS across all households)
//   working_class    → CLASSES instance with class_id=0
//   capitalist_class → CLASSES instance with class_id=1
households = country;  // Use country as search root for all household aggregations

// Initialize individual CLASS pointers for direct access
working_class = SEARCH_CND("class_id", 0);     // class_id=0 → workers
capitalist_class = SEARCH_CND("class_id", 1);  // class_id=1 → capitalists

if(working_class == NULL || capitalist_class == NULL)
    LOG("\nWARNING: CLASS pointers not initialized - check LSD structure");
else
{
    LOG("\nPhase 1: CLASSES structure initialized (households, working_class, capitalist_class)");
    // DEBUG: Verify pointer addresses
    LOG("\n  DEBUG: households=%p, country=%p, working_class=%p, capitalist_class=%p",
        households, country, working_class, capitalist_class);
    LOG("\n  DEBUG: households==country? %s", (households == country) ? "YES" : "NO");
}

// =========================================================================
// Phase 1: AUTOMATIC HOUSEHOLD REDISTRIBUTION
// Adjusts HOUSEHOLD counts in each CLASSES instance to match parameters
// =========================================================================
if(working_class != NULL && capitalist_class != NULL)
{
    // Read population parameters
    v[950] = VS(country, "country_total_population");      // Total households (e.g., 100)
    v[951] = VS(country, "capitalist_population_share");   // Capitalist share (e.g., 0.05 = 5%)
    v[952] = round(v[950] * v[951]);                       // Target capitalists
    v[954] = v[950] - v[952];                              // Target workers

    // Count current households in each class
    v[955] = COUNTS(working_class, "HOUSEHOLD");           // Current workers
    v[956] = COUNTS(capitalist_class, "HOUSEHOLD");        // Current capitalists

    LOG("\n========================================");
    LOG("\nHOUSEHOLD REDISTRIBUTION (Phase 1):");
    LOG("\n  Target Total: %.0f (Workers: %.0f, Capitalists: %.0f)", v[950], v[954], v[952]);
    LOG("\n  Current: Workers=%.0f, Capitalists=%.0f", v[955], v[956]);

    // Get template households for copying (first household in each class)
    cur1 = SEARCHS(working_class, "HOUSEHOLD");      // Worker template
    cur2 = SEARCHS(capitalist_class, "HOUSEHOLD");   // Capitalist template

    if(cur1 == NULL || cur2 == NULL)
    {
        LOG("\nERROR: No HOUSEHOLD template found in one or both CLASSES!");
    }
    else
    {
        // Adjust CAPITALIST count FIRST (so capitalists are created before workers)
        if(v[956] < v[952])
        {
            // Need to ADD capitalists
            for(v[957] = v[956]; v[957] < v[952]; v[957]++)
                ADDOBJ_EXS(capitalist_class, "HOUSEHOLD", cur2);
            LOG("\n  Added %.0f capitalists", v[952] - v[956]);
        }
        else if(v[956] > v[952])
        {
            // Need to DELETE excess capitalists (keep first one as template)
            v[958] = 0;
            CYCLE_SAFES(capitalist_class, cur, "HOUSEHOLD")
            {
                v[958]++;
                if(v[958] > v[952])
                    DELETE(cur);
            }
            LOG("\n  Deleted %.0f excess capitalists", v[956] - v[952]);
        }

        // Adjust WORKER count SECOND
        if(v[955] < v[954])
        {
            // Need to ADD workers
            for(v[957] = v[955]; v[957] < v[954]; v[957]++)
                ADDOBJ_EXS(working_class, "HOUSEHOLD", cur1);
            LOG("\n  Added %.0f workers", v[954] - v[955]);
        }
        else if(v[955] > v[954])
        {
            // Need to DELETE excess workers (keep first one as template)
            v[958] = 0;
            CYCLE_SAFES(working_class, cur, "HOUSEHOLD")
            {
                v[958]++;
                if(v[958] > v[954])
                    DELETE(cur);
            }
            LOG("\n  Deleted %.0f excess workers", v[955] - v[954]);
        }

        // Verify final counts
        v[955] = COUNTS(working_class, "HOUSEHOLD");
        v[956] = COUNTS(capitalist_class, "HOUSEHOLD");
        LOG("\n  Final: Workers=%.0f, Capitalists=%.0f, Total=%.0f", v[955], v[956], v[955] + v[956]);
    }
    LOG("\n========================================");
}

// =========================================================================
// Stage 3+: Initialize household parameters
// Now processes ALL households in BOTH CLASSES instances
// household_type is set based on which CLASSES instance contains them
// =========================================================================
if(households != NULL)
{
    // =========================================================================
    // MASTER HETEROGENEITY SWITCH
    // 0 = Homogeneous (representative agent) - for debugging/tuning
    // 1 = Heterogeneous (full model) - for final simulations
    // =========================================================================
    v[777] = VS(country, "switch_household_heterogeneity");

    // =========================================================================
    // POPULATION PARAMETERS (now handled by redistribution above)
    // Type assignment is based on parent CLASSES instance's class_id
    // =========================================================================
    LOG("\n========================================");
    LOG("\nHOUSEHOLD PARAMETER INITIALIZATION:");
    LOG("\n  (Type assigned from parent CLASSES class_id)");
    LOG("\n========================================");

    // Stage 4.1a: Read skill stddev (SAFEGUARD: if 0, skip lnorm)
    v[78] = VS(country, "household_skill_stddev");
    v[79] = -0.5 * pow(v[78], 2);  // Log-space μ for E[skill]=1.0

    // Stage 4.1b: Read autonomous consumption baseline
    v[83] = VS(country, "household_avg_autonomous_consumption_adjustment");

    // Stage 4.1c: Static import propensity baseline (class-independent)
    // History: 0.00=stable growth, 0.05=stable, 0.18=validated with 5 seeds
    // Stage 4.7 complete - proceeding to Stage 5 (inequality infrastructure)
    // TODO: Add country-level parameter with heterogeneity once validated
    v[500] = 0.18;

    // Stage 4.1d: Read liquidity preference baseline (used to derive household heterogeneity)
    v[89] = VS(country, "household_avg_liquidity_preference");

    // Stage 4.3e: Read Consumption Ratchet distribution parameters
    v[300] = VS(country, "switch_household_propensity_hysteresis");   // Mode: 0=static, 1=habits, 2=true hysteresis
    v[301] = VS(country, "household_habit_inflation_alpha");          // α⁺ for beta distribution (default 5.0)
    v[302] = VS(country, "household_habit_inflation_beta");           // β⁺ for beta distribution (default 1.0)
    v[303] = VS(country, "household_habit_deflation_alpha");          // α⁻ for beta distribution (default 2.0)
    v[304] = VS(country, "household_habit_deflation_beta");           // β⁻ for beta distribution (default 5.0)

    // Stage 5.1: Q-Exponential profit distribution parameters
    v[400] = VS(country, "household_profit_q");       // Entropic index (default 1.5, higher = more inequality)
    v[401] = VS(country, "household_profit_lambda");  // Scale parameter (default 1.0)

    // Stage 9: Propensity to evade parameters (not used in init, but read for logging)
    // household_propensity_evade is drawn from Beta(2,2) in the CYCLE loop

    v[80] = 0;  // skill_sum
    v[81] = 0;  // hh_count
    v[84] = 0;  // autonomous_consumption_sum
    v[88] = 0;  // import_propensity_sum
    v[92] = 0;  // liquidity_preference_sum
    v[98] = 0;  // worker_count
    v[99] = 0;  // capitalist_count
    v[305] = 0; // habit_persistence_sum (β)
    v[306] = 0; // habit_inflation_sum (λ⁺)
    v[307] = 0; // habit_deflation_sum (λ⁻)
    v[402] = 0; // raw_profit_share_sum (for normalization)
    v[96] = 0;  // worker_skill_sum (for skill-weighted deposit distribution)
    v[330] = 0; // propensity_evade_sum (Stage 9)

    // Use nested CYCLES to iterate through ALL households across BOTH classes
    // (CYCLES from country only finds siblings in the first CLASSES found)
    CYCLE(cur1, "CLASSES")
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        // =====================================================================
        // TYPE ASSIGNMENT FROM PARENT CLASSES INSTANCE
        // household_type = class_id of parent CLASSES (0=worker, 1=capitalist)
        // =====================================================================
        v[901] = VS(cur->up, "class_id");  // Get type from parent's class_id
        WRITES(cur, "household_type", v[901]);

        // Stage 5.2: Initialize employment state (all workers start employed)
        v[902] = (v[901] == 0) ? 1 : -1;  // Worker=1 (employed), Capitalist=-1
        WRITES(cur, "Household_Employment_Status", v[902]);
        WRITELLS(cur, "Household_Employment_Status", v[902], 0, 1);  // Set lag

        // =====================================================================
        // HETEROGENEITY-CONTROLLED PARAMETERS
        // When switch_household_heterogeneity = 0: Use fixed representative values
        // When switch_household_heterogeneity = 1: Use random distributions
        // =====================================================================

        // Stage 4.1a: Skill
        if(v[777] == 1 && v[78] > 0)
            v[82] = lnorm(v[79], v[78]);  // Heterogeneous: log-normal
        else
            v[82] = 1.0;  // Homogeneous: all skills = 1.0
        WRITES(cur, "household_skill", v[82]);
        v[80] += v[82];

        // Stage 4.1b: Autonomous consumption adjustment
        if(v[777] == 1 && v[83] > 0)
            v[85] = max(0, min(1, v[83] * norm(1.0, 0.20)));  // Heterogeneous
        else
            v[85] = v[83];  // Homogeneous: all = baseline (0.4)
        WRITES(cur, "household_autonomous_consumption_adjustment", v[85]);
        v[84] += v[85];

        // Stage 4.1c: Import propensity (already homogeneous - static 0.18)
        v[87] = v[500];
        WRITES(cur, "household_import_propensity", v[87]);
        v[88] += v[87];

        // Stage 4.1d: Liquidity preference (effective value derived from baseline)
        // Heterogeneous: draw from distribution centered on baseline
        // Homogeneous: all households get the baseline value
        if(v[777] == 1 && v[89] > 0)
            v[91] = max(0.05, min(0.95, v[89] * norm(1.0, 0.30)));  // Heterogeneous: ~N(baseline, baseline×0.3)
        else
            v[91] = v[89];  // Homogeneous: all = baseline
        WRITES(cur, "household_liquidity_preference", v[91]);
        v[92] += v[91];

        // Count workers and capitalists (for logging)
        if(v[901] == 0)
        {
            v[98]++;        // worker_count
            v[96] += v[82]; // worker_skill_sum (for skill-weighted distributions)
        }
        else
            v[99]++;  // capitalist_count

        // Stage 4.3e: Behavioral parameters for Consumption Ratchet
        // β (habit_persistence)
        if(v[777] == 1)
            v[308] = RND;  // Heterogeneous: uniform [0,1]
        else
            v[308] = 0.5;  // Homogeneous: representative value
        WRITES(cur, "household_habit_persistence", v[308]);
        v[305] += v[308];

        // λ⁺ (habit_inflation)
        if(v[777] == 1)
            v[309] = beta(v[301], v[302]);  // Heterogeneous: beta distribution
        else
            v[309] = 0.83;  // Homogeneous: E[Beta(5,1)] ≈ 0.833
        WRITES(cur, "household_habit_inflation", v[309]);
        v[306] += v[309];

        // λ⁻ (habit_deflation)
        if(v[300] == 2)
            v[310] = 0;  // Mode 2: permanent ratchet
        else if(v[777] == 1)
            v[310] = beta(v[303], v[304]);  // Heterogeneous: beta distribution
        else
            v[310] = 0.29;  // Homogeneous: E[Beta(2,5)] ≈ 0.286
        WRITES(cur, "household_habit_deflation", v[310]);
        v[307] += v[310];

        // Stage 5.1: Q-Exponential profit share (FIRST PASS - raw values)
        // Workers get 0, capitalists get q-exponential draw
        if(v[901] == 0)  // Worker
        {
            v[403] = 0;  // No profit share
        }
        else  // Capitalist
        {
            // Draw raw profit share from q-exponential
            if(v[777] == 1)  // Heterogeneous mode
                v[403] = qexponential(v[400], v[401]);  // Q-exp draw
            else  // Homogeneous mode
                v[403] = 1.0;  // Equal shares (will be normalized)
        }
        WRITES(cur, "household_profit_share", v[403]);  // Store raw (will normalize later)
        v[402] += v[403];  // Sum for normalization

        // Stage 9: Propensity to evade taxes (tax morale)
        // Beta distribution centered on country_avg_propensity_evade
        // Reparameterized: α = μ×κ, β = (1-μ)×κ where κ=4 (concentration)
        // μ = 0.5 → Beta(2,2); μ = 0.3 → Beta(1.2,2.8); μ = 0.7 → Beta(2.8,1.2)
        v[332] = VS(country, "country_avg_propensity_evade");  // μ (country baseline)
        v[333] = 4.0;  // κ (concentration parameter)
        v[334] = v[332] * v[333];         // α = μ × κ
        v[335] = (1.0 - v[332]) * v[333]; // β = (1-μ) × κ

        if(v[332] == 0 || v[332] == 1)  // Edge case: μ=0 or μ=1 makes Beta degenerate
            v[331] = v[332];  // Everyone gets exactly 0 or 1
        else if(v[777] == 1)  // Heterogeneous
            v[331] = beta(v[334], v[335]);  // Draw from Beta(α, β)
        else
            v[331] = v[332];  // Homogeneous: everyone at country average
        WRITES(cur, "household_propensity_evade", v[331]);
        v[330] += v[331];  // Sum for logging

        // Note: Household_Stock_Deposits initialized in later section (after class init)

        v[81]++;
    }
    }  // End CYCLE(cur1, "CLASSES")

    LOG("\nSTAGE 4: Initialized %.0f households (%.0f workers, %.0f capitalists)", v[81], v[98], v[99]);
    if(v[777] == 0)
        LOG("\n  >>> HOMOGENEOUS MODE (switch=0) - Representative agent for debugging <<<");
    else
        LOG("\n  >>> HETEROGENEOUS MODE (switch=1) - Full agent diversity <<<");
    LOG("\n  skill_stddev=%.3f, Avg skill=%.4f (target=1.0)", v[78], v[80] / max(1.0, v[81]));
    LOG("\n  autonomous_baseline=%.3f, Avg adjustment=%.4f", v[83], v[84] / max(1.0, v[81]));
    LOG("\n  import_propensity: Avg=%.4f (static=%.3f)",
        v[88] / max(1.0, v[81]), v[500]);
    LOG("\n  liquidity_baseline=%.3f, Avg liquidity_pref=%.4f", v[89], v[92] / max(1.0, v[81]));
    LOG("\n  Ratchet mode=%.0f: beta_avg=%.3f, lambda_up=%.3f, lambda_down=%.3f",
        v[300], v[305] / max(1.0, v[81]), v[306] / max(1.0, v[81]), v[307] / max(1.0, v[81]));
    LOG("\n  STAGE 9: propensity_evade ~ Beta(%.1f,%.1f), Avg=%.4f (target=%.2f)",
        v[334], v[335], v[330] / max(1.0, v[81]), v[332]);

    // =========================================================================
    // Stage 5.1: NORMALIZE Q-EXPONENTIAL PROFIT SHARES (SECOND PASS)
    // Ensures SUM(household_profit_share) = 1.0 for capitalists
    // =========================================================================
    if(v[402] > 0.01)  // Only if there are capitalists with positive shares
    {
        v[404] = 0;  // max_share (for Gini approximation)
        v[405] = 0;  // min_share
        v[406] = 1e10;  // Initialize min to large value

        CYCLE(cur1, "CLASSES")
        {
        CYCLES(cur1, cur, "HOUSEHOLD")
        {
            v[407] = VS(cur, "household_type");
            if(v[407] == 1)  // Capitalist
            {
                v[408] = VS(cur, "household_profit_share");
                v[409] = v[408] / v[402];  // Normalize: share / sum
                WRITES(cur, "household_profit_share", v[409]);

                // Track max/min for inequality reporting
                if(v[409] > v[404]) v[404] = v[409];  // max
                if(v[409] < v[406]) v[406] = v[409];  // min
            }
        }
        }  // End CYCLE(cur1, "CLASSES")

        // Log profit distribution statistics
        v[410] = (v[99] > 0) ? 1.0 / v[99] : 0;  // Equal share reference
        v[411] = v[404] / max(0.001, v[410]);    // Max/equal ratio (concentration measure)
        LOG("\n  STAGE 5.1: Q-Exponential profit distribution (q=%.2f, λ=%.2f)", v[400], v[401]);
        LOG("\n    Capitalists: %.0f, Equal share: %.4f", v[99], v[410]);
        LOG("\n    Max share: %.4f (%.1fx equal), Min share: %.6f", v[404], v[411], v[406]);
        LOG("\n    Top capitalist receives %.1f%% of total profits", v[404] * 100);
    }
    else
    {
        LOG("\n  WARNING: No capitalists with positive profit shares!");
    }
}


//COUNTRY PARAMETERS
v[0]=VS(country, "annual_frequency");
v[1]=VS(country, "country_initial_depreciation_share_GDP");
v[2]=VS(country, "country_initial_government_share_GDP");
v[3]=VS(country, "country_initial_exports_share_GDP");
//CONSUMPTION PARAMETERS
v[10]=VS(consumption, "sector_initial_depreciation_scale");
v[11]=VS(consumption, "sector_investment_frequency");
v[12]=VS(consumption, "sector_number_object_firms");
v[13]=VS(consumption, "sector_initial_price");
v[14]=VS(consumption, "sector_input_tech_coefficient");
v[15]=VS(consumption, "sector_initial_propensity_import_inputs");
v[16]=VS(consumption, "sector_initial_exports_share");
v[17]=VS(consumption, "sector_initial_external_price");
//CAPITAL PARAMETERS
v[20]=VS(capital, "sector_initial_depreciation_scale");
v[21]=VS(capital, "sector_investment_frequency");
v[22]=VS(capital, "sector_number_object_firms");
v[23]=VS(capital, "sector_initial_price");
v[24]=VS(capital, "sector_input_tech_coefficient");
v[25]=VS(capital, "sector_initial_propensity_import_inputs");
v[26]=VS(capital, "sector_initial_exports_share");
v[27]=VS(capital, "sector_initial_external_price");
//INPUT PARAMETERS
v[30]=VS(input, "sector_initial_depreciation_scale");
v[31]=VS(input, "sector_investment_frequency");
v[32]=VS(input, "sector_number_object_firms");
v[33]=VS(input, "sector_initial_price");
v[34]=VS(input, "sector_input_tech_coefficient");
v[35]=VS(input, "sector_initial_propensity_import_inputs");
v[36]=VS(input, "sector_initial_exports_share");
v[37]=VS(input, "sector_initial_external_price");
//EXTERNAL SECTOR PARAMETERS
v[40]=VS(external, "external_interest_rate");
v[41]=VS(external, "initial_external_income_scale");
v[42]=VS(external, "external_capital_flow_adjustment");
v[43]=VS(external, "initial_reserves_ratio");
v[44]=VS(external, "initial_exchange_rate");
//FINANCIAL PARAMETERS
v[50]=VS(financial, "cb_quarterly_nominal_interest_rate");
v[51]=VS(financial, "fs_initial_leverage");
v[52]=VS(financial, "fs_spread_deposits");
v[53]=VS(financial, "fs_spread_short_term");
v[54]=VS(financial, "fs_spread_long_term");
v[55]=VS(financial, "fs_risk_premium_short_term");
v[56]=VS(financial, "fs_risk_premium_long_term");
v[57]=VS(financial, "fs_number_object_banks");
v[58]=VS(financial, "cb_minimum_capital_ratio");
v[59]=VS(financial, "fs_sensitivity_debt_rate");
//GOVERNMENT PARAMETERS
v[60]=VS(government, "government_initial_debt_gdp_ratio");
v[61]=VS(government, "government_initial_share_consumption");
v[62]=VS(government, "government_initial_share_capital");
v[63]=VS(government, "government_initial_share_input");
//CENTRAL BANK PARAMETERS
v[70]=VS(centralbank, "cb_target_annual_inflation");
	
	if(V("switch_monetary_policy")==2)			//smithin rule
		v[71]=v[70]/v[0];	
	else if(V("switch_monetary_policy")==3)		//pasinetti rule
		v[71]=v[70]/v[0];
	else if(V("switch_monetary_policy")==4)		//kansas city rule.
		v[71]=v[70]/v[0];
	else					//taylor rule or fixed monetary policy
		v[71]=v[50];		

	v[100]=(((v[20]*v[22]/v[21])+(v[30]*v[32]/v[31])+(v[10]*v[12]/v[11]))*v[23])/v[1];				//nominal GDP
		LOG("\nNominal GDP is %f.",v[100]);	
		
	//GOVERNMENT INTERMEDIATE CALCULATION
	v[101]=v[100]*v[60];								//government debt
	v[102]=v[71];                                       //interest rate on government debt
	v[103]=v[102]*v[101];								//government interest payment
	v[104]=v[2]*v[100];									//government expenses	
	v[105]=v[103]+v[104];								//government total taxes
	v[106]=v[104]*v[61];								//government nominal consumption	
	v[107]=v[104]*v[62];								//government nominal investment	
	v[108]=v[104]*v[63];								//government nominal inputs
	v[109]=v[106]/v[13];								//government real consumption
	v[110]=v[107]/v[23];								//government real investment
	v[111]=v[108]/v[33];								//government real inputs
	v[112]=v[104]-v[106]-v[107]-v[108];					//government wages
	v[113]=v[103]/v[100];								//government surplus rate target
	
	//LOG PLOTTING GOVERNMENT VARIABLES
	LOG("\nGovernment Debt is %f.",v[101]);	
	LOG("\nBasic Interest Rate is %f.",v[102]);
	LOG("\nGovernment Interest Payment is %f.",v[103]);	
	LOG("\nGovernment Primary Expenses is %f.",v[104]);
	LOG("\nTotal Taxes is %f.",v[105]);	
	LOG("\nGovernment Nominal Consumption is %f.",v[106]);	
	LOG("\nGovernment Nominal Investment %f.",v[107]);
	LOG("\nGovernment Nominal Inputs is %f.",v[108]);
	LOG("\nGovernment Nominal Wages is %f.",v[112]);		
	LOG("\nGovernment Surplus Target is %f.",v[113]);
			
	//WRITTING GOVERNMENT LAGGED VALUES
	WRITELLS(government, "Government_Desired_Wages", v[112], 0, 1);
	WRITELLS(government, "Government_Desired_Unemployment_Benefits", 0, 0, 1);
	WRITELLS(government, "Government_Desired_Consumption", v[106], 0, 1);
	WRITELLS(government, "Government_Desired_Investment", v[107], 0, 1);
	WRITELLS(government, "Government_Effective_Investment", v[107], 0, 1);
	WRITELLS(government, "Government_Desired_Inputs", v[108], 0, 1);
	WRITELLS(government, "Government_Surplus_Rate_Target", v[113], 0, 1);
	for(i=1;i<=v[0]+1;i++)WRITELLS(government, "Government_Debt", v[101], 0, i);
	WRITELLS(government, "Government_Total_Taxes", v[105], 0, 1);
	WRITELLS(government, "Government_Max_Expenses_Ceiling", v[104], 0, 1);//olhar depois
	WRITELLS(government, "Government_Max_Expenses_Surplus", v[104], 0, 1);//olhar depois
	WRITELLS(government, "Government_Max_Expenses", v[104], 0, 1);//olhar depois
	for(i=1;i<=v[0]+1;i++) 	WRITELLS(government, "Government_Debt_GDP_Ratio", v[60], 0, i);
	for(i=1;i<=v[0];i++) 	WRITELLS(government, "Government_Effective_Expenses", v[104], 0, i);

	//EXTERNAL INTERMEDIATE CALCULATION
	v[120]=v[41]*v[100];						        //external nominal income
	v[121]=v[44]*v[100]*v[42]*(v[71]-v[40]);			//capital flows
	v[122]=v[43]*v[100];								//international reserves
	v[123]=v[100]*v[3];									//country nominal exports
	v[124]=v[123]+v[121];								//country nominal imports
	v[125]=v[123]*v[16];								//country nominal consuption exports
	v[126]=v[123]*v[26];								//country nominal capital exports
	v[127]=v[123]*v[36];								//country nominal input exports
	v[128]=v[125]/v[13];								//country real consumption exports
	v[129]=v[126]/v[23];								//country real capital exports
	v[130]=v[127]/v[33];								//country real input exports
	
	//LOG PLOTTING EXTERNAL VARIABLES
	LOG("\nExternal Nominal Income is %f.",v[120]);	
	LOG("\nCapital Flows is %f.",v[121]);
	LOG("\nInternational Reserves is %f.",v[122]);	
	LOG("\nNominal Exports is %f.",v[123]);
	LOG("\nNominal Imports is %f.",v[124]);	
	LOG("\nNominal Consumption Exports is %f.",v[125]);	
	LOG("\nNominal Capital Exports %f.",v[126]);
	LOG("\nNominal Input Exports is %f.",v[127]);

	//SECTORAL DEMAND CALCULATION
	v[140]=v[100]*(1-v[1]-v[2]-v[3])-v[103];														//nominal domestic consumption
	v[141]=(v[140]/v[13])+v[128]+v[109];															//real consumption demand
	v[142]=(v[20]*v[22]/v[21])+(v[30]*v[32]/v[31])+(v[10]*v[12]/v[11])+v[129]+v[110];				//real capital demand
	v[143]=(v[141]*v[14]*(1-v[15])+v[142]*v[24]*(1-v[25])+v[130]+v[111])/(1-v[34]*(1-v[35]));		//real input demand
	WRITES(consumption, "sector_initial_demand", v[141]);
	WRITES(capital, "sector_initial_demand", v[142]);
	WRITES(input, "sector_initial_demand", v[143]);
	LOG("\nReal Consumption Demand is %f.",v[141]);	
	LOG("\nReal Capital Demand %f.",v[142]);
	LOG("\nReal Input Demand is %f.",v[143]);

	v[270]=WHTAVE("sector_initial_price", "sector_initial_demand")/SUM("sector_initial_demand");	//average price
	
	//WRITTING EXTERNAL SECTOR LAGGED VALUES
	WRITELLS(external, "External_Real_Income", v[120]/v[270], 0, 1);
	WRITELLS(external, "External_Real_Income", v[120]/v[270], 0, 2);
	WRITELLS(external, "Country_Exchange_Rate", v[44], 0, 1);
	WRITELLS(external, "Country_Nominal_Exports", v[123], 0, 1);
	WRITELLS(external, "Country_Nominal_Imports", v[124], 0, 1);
	WRITELLS(external, "Country_International_Reserves", v[122], 0, 1);
	WRITELLS(external, "Country_Trade_Balance", v[123]-v[124], 0, 1);
	WRITELLS(external, "Country_Capital_Flows", v[121], 0, 1);
	WRITELLS(external, "Country_International_Reserves_GDP_Ratio", v[43], 0, 1);
	WRITELLS(external, "Country_External_Debt", 0, 0, 1);
	
	
v[210]=v[211]=v[212]=v[213]=v[214]=v[215]=v[216]=v[217]=v[218]=v[219]=v[226]=0;
CYCLE(cur, "SECTORS")
{
	v[150]=VS(cur, "sector_initial_demand");
	v[151]=VS(cur, "sector_investment_frequency");
	v[152]=VS(cur, "sector_number_object_firms");
	v[153]=VS(cur, "sector_initial_price");
	v[154]=VS(cur, "sector_input_tech_coefficient");
	v[155]=VS(cur, "sector_initial_propensity_import_inputs");
	v[156]=VS(cur, "sector_desired_degree_capacity_utilization");
	v[157]=VS(cur, "sector_initial_external_price");
	v[158]=VS(cur, "sector_capital_output_ratio");
	v[159]=VS(cur, "sector_initial_productivity");
	v[160]=VS(cur, "sector_indirect_tax_rate");
	v[161]=VS(cur, "sector_rnd_revenue_proportion");
	v[162]=VS(cur, "sector_initial_debt_rate");
	v[163]=VS(cur, "sector_initial_liquidity_preference");
	v[164]=VS(cur, "sector_initial_profit_rate");
	v[165]=VS(cur, "sector_capital_duration");
	v[166]=VS(cur, "sector_initial_depreciation_scale");
	v[167]=VS(cur, "sector_initial_quality");
	v[168]=VS(cur, "sector_desired_inventories_proportion");
	v[169]=VS(cur, "sector_price_frequency");
	
	v[170]=v[150]*v[153];											//sector revenue
	v[171]=v[150]*v[153]*v[160];									//sector taxation
	v[172]=v[150]*v[153]*(1-v[160])*v[161];							//sector rnd expenses
	v[173]=v[150]*v[154]*(1-v[155])*v[33];							//sector domestic input expenses
	v[174]=v[150]*v[154]*v[155]*v[37]*v[44];						//sector imported input expenses
	v[175]=v[173]+v[174];											//sector total input expenses
	v[176]=v[150]*v[164];											//sector gross profits
	
	v[177]=v[150]/v[156];											//sector desired capacity
	v[178]=v[177]/v[152];											//firm desired capacity
	v[179]=ROUND(v[178]*v[158], "UP");								//firm number capitals
	v[180]=v[179]*v[152];											//sector number capitals
	v[181]=v[180]*v[23];											//sector nominal capital
	v[182]=v[163]*v[181];											//sector stock deposits
	v[183]=v[181]*(1+v[163])*v[162];								//sector stock loans
	v[184]=v[102]+v[54]+v[56]*v[162];								//sector avg interest rate long term
	v[185]=v[184]*v[183];											//sector interest payment
	v[186]=v[182]*max(0,v[102]-v[52]);								//sector interest receivment
	v[187]=v[170]-v[171]-v[172]-v[175]-v[185]+v[186]-v[176];		//sector wage payment
	v[188]=v[187]*v[159]/v[150];									//sector wage rate
	v[189]=(v[188]/v[159])+(v[175]/v[150]);							//sector unit variable cost
	v[190]=v[153]/v[189];											//sector markup
	v[191]=v[150]/v[159];											//sector employment
	v[192]=v[183]/v[165];											//sector amortization expenses
	v[193]=v[23]*(v[166]*v[152]/v[151]);							//sector investment expenses
	v[194]=v[193]-v[192];											//sector retained profits
	v[195]=v[176]-v[194];											//sector distributed profits
	v[196]=v[195]/v[176];											//sector profit distribution rate
	v[197]=v[180]*v[158];											//sector productive capacity
	v[198]=v[150]/v[197];											//sector capacity utilization
	v[199]=(VS(cur,"sector_initial_exports_share")*v[123]/v[153])/(pow((v[157]*v[44]/v[153]),VS(cur,"sector_exports_elasticity_income"))*pow((v[120]/v[270]),VS(cur,"sector_exports_elasticity_income")));

	//WRITTING SECTOR LAGGED VALUES
	WRITES(cur, "sector_exports_coefficient", v[199]);
	WRITES(cur, "sector_desired_degree_capacity_utilization", v[198]);
	WRITES(cur, "sector_profits_distribution_rate", v[196]);
	WRITES(cur, "sector_desired_market_share", 2/v[152]);
	WRITELLS(cur, "Sector_External_Price", v[157], 0, 1);
	WRITELLS(cur, "Sector_Productive_Capacity", v[197], 0, 1);
	
	WRITELLS(cur, "Sector_Idle_Capacity", 1-v[198], 0, 1);
	WRITELLS(cur, "Sector_Productive_Capacity_Available", 0, 0, 1);
	WRITELLS(cur, "Sector_Number_Firms", v[152], 0, 1);
	WRITELLS(cur, "Sector_Avg_Productivity", v[159], 0, 1);
	WRITELLS(cur, "Sector_Max_Productivity", v[159], 0, 1);
	WRITELLS(cur, "Sector_Avg_Wage", v[188], 0, 1);
	WRITELLS(cur, "Sector_Max_Quality", v[167], 0, 1);
	WRITELLS(cur, "Sector_Propensity_Import_Inputs", v[155], 0, 1);
	WRITELLS(cur, "Sector_Exports_Share", (VS(cur,"sector_initial_exports_share")*v[123]/v[153])/v[150], 0, 1);
	for(i=1;i<=v[0]+1;i++) WRITELLS(cur, "Sector_Capacity_Utilization", v[198], 0, 1);
	for(i=1;i<=v[0]+1;i++) WRITELLS(cur, "Sector_Avg_Price", v[153], 0, i);
	for(i=1;i<=v[0]+1;i++) WRITELLS(cur, "Sector_Avg_Quality", v[167], 0, i);
	for(i=1;i<=v[0]+1;i++) WRITELLS(cur, "Sector_Employment", v[191], 0, i);
	for(i=1;i<=v[151];i++) WRITELLS(cur, "Sector_Demand_Met", 0, 0, i);
	for(i=1;i<=v[151];i++) WRITELLS(cur, "Sector_Demand_Met_By_Imports", 1, 0, i);
	for(i=1;i<=v[151];i++) WRITELLS(cur, "Sector_Effective_Orders", v[150], 0, i);
	
	LOG("\nSector %.0f",SEARCH_INST(cur));LOG(" Desired Capacity Uilization is %f.",v[198]);
	LOG("\nSector %.0f",SEARCH_INST(cur));LOG(" Profit Distribution Rate is %f.",v[196]);
	LOG("\nSector %.0f",SEARCH_INST(cur));LOG(" Wage Rate is %f.",v[188]);
	LOG("\nSector %.0f",SEARCH_INST(cur));LOG(" Markup is %f.",v[190]);
	LOG("\nSector %.0f",SEARCH_INST(cur));LOG(" Exports Coefficient is %f.",v[199]);
	LOG("\nSector %.0f",SEARCH_INST(cur));LOG(" Desired Market Share is %f.",2/v[152]);
	
	//WRITTING FIRM LAGGED VALUES
	cur1=SEARCHS(cur, "FIRMS");																	
	WRITES(cur1, "firm_date_birth", 0);   
	WRITELLS(cur1, "Firm_Effective_Market_Share", 1/v[152], 0, 1);
	WRITELLS(cur1, "Firm_Avg_Productivity", v[159], 0, 1);
	WRITELLS(cur1, "Firm_Price", v[153], 0, 1);
	WRITELLS(cur1, "Firm_Quality", v[167], 0, 1);
	WRITELLS(cur1, "Firm_Competitiveness", 1, 0, 1);
	WRITELLS(cur1, "Firm_Stock_Inventories", v[150]*v[168]/v[152], 0, 1);
	WRITELLS(cur1, "Firm_Delivery_Delay", 0, 0, 1);
	WRITELLS(cur1, "Firm_Stock_Deposits", v[182]/v[152], 0, 1);
	WRITELLS(cur1, "Firm_Wage", v[188], 0, 1);
	WRITELLS(cur1, "Firm_Desired_Markup", v[190], 0, 1);
	WRITELLS(cur1, "Firm_Avg_Debt_Rate", v[162], 0, 1);
	WRITELLS(cur1, "Firm_Max_Debt_Rate", 2*v[162], 0, 1);
	WRITELLS(cur1, "Firm_Stock_Inputs", v[150]*v[154]/v[152], 0, 1);
	WRITELLS(cur1, "Firm_Liquidity_Preference", v[163], 0, 1);
	WRITELLS(cur1, "Firm_Capital", v[181]/v[152], 0, 1);
	WRITELLS(cur1, "Firm_Stock_Loans", v[183]/v[152], 0, 1);
	for(i=1;i<=v[151];i++) 		WRITELLS(cur1, "Firm_Demand_Capital_Goods_Expansion", 0, 0, i);
	for(i=1;i<=v[151];i++) 		WRITELLS(cur1, "Firm_Demand_Capital_Goods_Replacement", 0, 0, i);
	for(i=1;i<=v[151];i++) 		WRITELLS(cur1, "Firm_Frontier_Productivity", v[159], 0, i);
	for(i=1;i<=v[151];i++) 		WRITELLS(cur1, "Firm_Productive_Capacity", v[197]/v[152], 0, i);
	for(i=1;i<=v[151];i++) 		WRITELLS(cur1, "Firm_Interest_Payment", v[185]/v[152], 0, i);
	for(i=1;i<=v[151];i++)		WRITELLS(cur1, "Firm_Debt_Rate", v[162], 0, i);
	for(i=1;i<=v[151];i++) 		WRITELLS(cur1, "Firm_Net_Profits", v[176]/v[152], 0, i);
	for(i=1;i<=v[151]-1;i++)	WRITELLS(cur1, "Firm_Effective_Orders_Capital_Goods", v[150]/v[152], 0, i);
	for(i=1;i<=2*v[151]-1;i++)	WRITELLS(cur1, "Firm_Effective_Orders", v[150]/v[152], 0, i);
	for(i=1;i<=v[169];i++) 		WRITELLS(cur1, "Firm_Market_Share", 1/v[152], 0, i);
	for(i=1;i<=v[0]+1;i++) 		WRITELLS(cur1, "Firm_Avg_Productivity", v[159], 0, i);
	
	//WRITTING CAPITAL LAGGED VALUES
	cur2=SEARCHS(cur1, "CAPITALS");														
	WRITELLS(cur2, "Capital_Good_Acumulated_Production", 0, 0, 1);      				
	WRITES(cur2, "capital_good_productive_capacity", 1/v[158]);     					
	WRITES(cur2, "capital_good_productivity_initial", v[159]);       		  			
	WRITES(cur2, "capital_good_to_replace", 0);
	WRITES(cur2, "capital_good_date_birth", 0);
	WRITES(cur2, "id_capital_good_number", 1);    
	
	//CREATING FIRM OBJECTS
	for(i=1; i<=(v[152]-1); i++)															
	cur4=ADDOBJ_EXLS(cur,"FIRMS", cur1, 0);
	CYCLES(cur, cur1, "FIRMS")                                                 				
		{
			v[200]=SEARCH_INSTS(cur, cur1);
			WRITES(cur1, "firm_id", v[200]);                         	
			v[201]=v[200]/(v[152]/v[57]);
			//WRITES(cur1, "firm_bank", ROUND(v[201], "UP"));
			WRITES(cur1, "firm_bank", uniform_int(1,v[57]));
			
			//WRITTING FIRM_LOANS LAGGED VALUES
			cur2=SEARCHS(cur1, "FIRM_LOANS");
			WRITES(cur2, "id_firm_loan_long_term", 1);     					
			WRITES(cur2, "id_firm_loan_short_term", 0);   
			WRITES(cur2, "firm_loan_total_amount", v[183]/v[152]);			
			WRITES(cur2, "firm_loan_interest_rate", v[184]);
			WRITES(cur2, "firm_loan_fixed_amortization", v[192]/v[152]);
			WRITES(cur2, "firm_loan_fixed_object", 0);
			
			//CREATING CAPITAL OBJECTS
			cur2=SEARCHS(cur1, "CAPITALS");   
			for(i=1; i<=(v[179]-1); i++)                        								
			{                                 			
			cur3=ADDOBJ_EXLS(cur1,"CAPITALS", cur2, 0);			                       		
			WRITES(cur3, "id_capital_good_number", (i+1));										
			}
			
			CYCLES(cur1, cur2, "CAPITALS")                                            			
				{
				v[202]=fmod(v[200]+v[151], v[151]);
				v[203]=VS(cur2, "id_capital_good_number");
				v[204]=v[202]+(v[203]-1)*v[151];			
				WRITES(cur2, "capital_good_depreciation_period", v[204]);
				}
		}

v[210]+=v[171];														//total indirect taxation
v[211]+=v[172];														//total RND expenses
v[212]+=v[174];														//total imported imput expenses
v[213]+=v[182];														//total firms stock deposits
v[214]+=v[183];														//total stock loans
v[215]+=v[185];														//total interest payment
v[216]+=v[186];														//total interest receivment
v[217]+=v[187];														//total wage payment
v[218]+=v[195];														//total distributed profits
v[219]+=v[181];														//total nominal capital
v[226]+=(v[193]-v[194]);											//total demand loans
}

	v[220]=v[214]/(v[213]+v[219]);									//average debt rate
	v[221]=v[214]/v[57];											//bank stock of debt
	v[222]=v[221]*(v[58]+v[59]*v[220]);								//bank initial accumulated profits
	v[223]=v[215]-v[216]+v[103];									//financial sector profits
	v[224]=v[214]/v[51];											//total stock deposits
	v[225]=v[224]-v[213];											//total class stock deposits

	//WRITTING FINANCIAL LAGGED VALUES
	WRITELLS(centralbank, "Central_Bank_Basic_Interest_Rate", v[102], 0, 1);
	WRITELLS(financial, "Financial_Sector_Avg_Competitiveness", 1, 0, 1);
	WRITELLS(financial, "Financial_Sector_Avg_Interest_Rate_Short_Term", v[102]+v[53], 0, 1);
	WRITELLS(financial, "Financial_Sector_Avg_Interest_Rate_Long_Term", v[102]+v[54], 0, 1);
	WRITELLS(financial, "Financial_Sector_Total_Stock_Loans_Growth", 0, 0, 1);
	WRITELLS(financial, "Financial_Sector_Total_Stock_Loans", v[214], 0, 1);
	
	//CREATING BANK OBJECTS
	cur1=SEARCHS(financial, "BANKS");
	for(i=1; i<=(v[57]-1); i++)																
	cur2=ADDOBJ_EXLS(financial,"BANKS", cur1, 0);

	//WRITTING BANK LAGGED VALUES
	CYCLES(financial, cur1, "BANKS")                                                 				
		{												
		WRITES(cur1, "bank_id", SEARCH_INSTS(root, cur1)); 
		WRITELLS(cur1, "Bank_Market_Share", 1/v[57], 0, 1);
		for(i=1;i<=v[0]+1;i++) WRITELLS(cur1, "Bank_Default_Share", 0, 0, i);
		WRITELLS(cur1, "Bank_Accumulated_Defaulted_Loans", 0, 0, 1);
		WRITELLS(cur1, "Bank_Total_Stock_Loans", v[214]/v[57], 0, 1);
		WRITELLS(cur1, "Bank_Competitiveness", 1, 0, 1);
		WRITELLS(cur1, "Bank_Demand_Met", 1, 0, 1);
		WRITELLS(cur1, "Bank_Demand_Loans", v[226]/v[57], 0, 1);
		WRITELLS(cur1, "Bank_Desired_Short_Term_Spread", v[53], 0, 1);
		WRITELLS(cur1, "Bank_Desired_Long_Term_Spread", v[54], 0, 1);
		WRITELLS(cur1, "Bank_Interest_Rate_Short_Term", v[102]+v[53], 0, 1);
		WRITELLS(cur1, "Bank_Interest_Rate_Long_Term", v[102]+v[54], 0, 1);
		WRITELLS(cur1, "Bank_Accumulated_Profits", v[222], 0, 1);
		}
		
	//AGGREGATE INTERMEDIATE VARIABLES
	v[230]=v[211]+v[217]+v[112];											//total wages
	if(V("switch_fixed_bank_profit_distribution")==1)
		v[231]=v[218]+VS(financial, "fs_profit_distribution_rate")*v[223];	//total distributed profits
	else
		v[231]=v[218]+v[223];
	v[232]=v[230]+v[231];													//total households gross income	
	v[233]=v[105]-v[210];													//total income taxation
	v[235]=v[124]-v[212];													//total imported consumption expenses
	
	// Stage 5.5: Using single country-level tax rate instead of class-weighted averages
	v[290] = VS(country, "household_income_tax_rate");  // Single tax rate for all households

	if(V("switch_class_tax_structure")==0)							    	//taxation structure = no tax
		v[280]=0;
	if(V("switch_class_tax_structure")==1)									//taxation structure = only wages
		v[280]=v[290]*v[230];
	if(V("switch_class_tax_structure")==2)									//taxation structure = only profits
		v[280]=v[290]*v[231];
	if(V("switch_class_tax_structure")==3)									//taxation structure = profits and wages
		v[280]=v[290]*v[231]+v[290]*v[230];
	if(V("switch_class_tax_structure")==4)									//taxation structure = profits, wages and interest
		v[280]=v[290]*v[231]+v[290]*v[230]+v[290]*max(0,v[102]-v[52])*v[225];
	LOG("\nPseudo Taxation %f.0",v[280]);
	LOG("\nTaxation %f.0",v[233]);
	v[281]=v[233]/v[280];

	// =========================================================================
	// Stage 5.5: Compute aggregate consumption values without CLASS cycle
	// These values are needed for household initialization
	// =========================================================================

	// Total disposable income = gross income - taxes
	v[245] = v[232] - v[233];  // = (wages + profits) - taxes

	// Compute weighted average propensity to spend from households
	// Workers ~0.9, Capitalists ~0.5, weighted by population
	// NOTE: Using type-based defaults since Household_Propensity_to_Spend is now a variable
	//       (variables cannot be read at t=0 - not yet computed)
	v[240] = 0;  // Sum of propensity
	v[241] = 0;  // Count
	if(households != NULL)
	{
		CYCLE(cur1, "CLASSES")
		{
		CYCLES(cur1, cur, "HOUSEHOLD")
		{
			// Read household_type parameter (0=worker, 1=capitalist)
			v[244] = VS(cur, "household_type");
			// Type-based propensity defaults (Post-Keynesian: workers consume more)
			v[242] = (v[244] == 0) ? 0.9 : 0.5;  // Workers=0.9, Capitalists=0.5
			v[240] += v[242];
			v[241] += 1;
		}
		}  // End CYCLE(cur1, "CLASSES")
	}
	v[243] = (v[241] > 0) ? (v[240] / v[241]) : 0.7;  // Average propensity (default 0.7)

	// Total induced expenses = disposable_income * avg_propensity
	v[246] = v[245] * v[243];

	// v[235] = total imported consumption expenses (already computed)
	// Total induced domestic consumption = induced_expenses - imports
	v[251] = v[246] - v[235];

	// Total induced savings = disposable_income - induced_expenses
	v[252] = v[245] - v[246];

	LOG("\n[Stage 5.5] Aggregate consumption (CLASS-free calculation):");
	LOG("\n  Disposable Income: %.2f, Avg Propensity: %.4f", v[245], v[243]);
	LOG("\n  Induced Expenses: %.2f, Induced Domestic Consumption: %.2f", v[246], v[251]);

	// Total autonomous consumption = total consumption - induced domestic consumption
	v[253] = v[140] - v[251];  // total autonomous consumption
	// NOTE: Class_Real_Autonomous_Consumption CYCLE removed (Stage 5.5)

// =========================================================================
// Phase D: Initialize Household_Real_Autonomous_Consumption EGALITARIAN
// Equal per-capita distribution (like target model) - subsistence needs
// =========================================================================
if(households != NULL)
{
    // v[253] = total autonomous consumption (already computed at line 722)
    // v[13] = price index (for real terms)
    // v[950] = total population (from earlier)

    // EGALITARIAN: equal per-capita (same for all households)
    v[600] = v[253] / max(1.0, v[950]);  // Per-capita nominal autonomous consumption
    v[601] = v[600] / v[13];             // Per-capita real autonomous consumption

    // Initialize each household's autonomous consumption
    v[610] = 0;  // Sum for verification
    CYCLE(cur1, "CLASSES")
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        // Individual adjustment factor (heterogeneous taste)
        v[608] = VS(cur, "household_autonomous_consumption_adjustment");

        // Egalitarian: same base for everyone, modified by individual adjustment
        v[609] = v[601] * v[608];
        v[609] = max(0, v[609]);  // Ensure non-negative

        WRITELLS(cur, "Household_Real_Autonomous_Consumption", v[609], 0, 1);
        v[610] += v[609];
    }
    }  // End CYCLE(cur1, "CLASSES")

    LOG("\n[Phase D] Initialized Household_Real_Autonomous_Consumption (EGALITARIAN):");
    LOG("\n  Total autonomous (nominal): %.4f", v[253]);
    LOG("\n  Per-capita (real): %.4f", v[601]);
    LOG("\n  Total household autonomous (real): %.4f", v[610]);

    // =========================================================================
    // Phase D.2: Initialize Household_Reference_Income (Skill/Profit-Share Weighted)
    // Mirrors deposit distribution logic: historical income → accumulated wealth
    // Workers: skill-weighted share of worker pool
    // Capitalists: profit_share-weighted share of capitalist pool
    // =========================================================================
    // Total real disposable income = (Wages + Profits - Taxes) / Price
    // NOTE: Direct calculation replaces class pointer reads (Stage 5.5)
    v[623] = (v[230] + v[231] - v[233]) / max(0.01, v[13]);  // Total real disposable income

    // Inter-class distribution (same as deposits)
    v[630] = VS(country, "initial_capitalist_deposit_share");  // Capitalist share (e.g., 0.60)
    v[631] = 1 - v[630];  // Worker share (e.g., 0.40)

    // v[96] = worker_skill_sum (computed in first loop)
    v[632] = max(0.001, v[96]);  // Prevent division by zero

    // Initialize each household's reference income
    v[625] = 0;  // Sum for verification
    CYCLE(cur2, "CLASSES")
    {
    CYCLES(cur2, cur, "HOUSEHOLD")
    {
        v[626] = VS(cur, "household_type");

        if(v[626] == 0)  // Worker
        {
            // Skill-weighted share of worker income pool
            v[633] = VS(cur, "household_skill");
            v[624] = v[623] * v[631] * (v[633] / v[632]);
        }
        else  // Capitalist
        {
            // Profit_share-weighted share of capitalist income pool
            v[627] = VS(cur, "household_profit_share");
            v[624] = v[623] * v[630] * v[627];
        }

        v[624] = max(0.01, v[624]);  // Ensure positive
        WRITELLS(cur, "Household_Reference_Income", v[624], 0, 1);
        v[625] += v[624];
    }
    }  // End CYCLE(cur2, "CLASSES")

    LOG("\n[Phase D.2] Initialized Household_Reference_Income (Skill/Profit-Share Weighted):");
    LOG("\n  Total disposable income: %.4f", v[623]);
    LOG("\n  Capitalist share: %.2f%%, Worker share: %.2f%%", v[630]*100, v[631]*100);
    LOG("\n  Total household reference income: %.4f", v[625]);

    // =========================================================================
    // Phase C: Initialize Household_Stock_Deposits (Skill/Profit-Share Weighted)
    // Workers: skill-weighted share of worker deposit pool
    // Capitalists: profit_share-weighted share of capitalist deposit pool
    // Inter-class split controlled by initial_capitalist_deposit_share parameter
    // =========================================================================
    // v[225] = total class stock deposits (already computed at line 600)
    v[700] = v[225];  // Total deposits to distribute

    // Inter-class distribution (exogenous parameter)
    v[701] = VS(country, "initial_capitalist_deposit_share");  // Capitalist share (e.g., 0.60)
    v[702] = 1 - v[701];  // Worker share (e.g., 0.40)

    // v[96] = worker_skill_sum (computed in first loop)
    v[703] = max(0.001, v[96]);  // Prevent division by zero

    v[705] = 0;  // Sum for verification
    CYCLE(cur2, "CLASSES")
    {
    CYCLES(cur2, cur, "HOUSEHOLD")
    {
        v[706] = VS(cur, "household_type");

        if(v[706] == 0)  // Worker
        {
            // Skill-weighted share of worker deposit pool
            v[708] = VS(cur, "household_skill");
            v[704] = v[700] * v[702] * (v[708] / v[703]);
        }
        else  // Capitalist
        {
            // Profit_share-weighted share of capitalist deposit pool
            v[707] = VS(cur, "household_profit_share");
            v[704] = v[700] * v[701] * v[707];
        }

        v[704] = max(0, v[704]);  // Ensure non-negative
        WRITES(cur, "Household_Stock_Deposits", v[704]);
        WRITELLS(cur, "Household_Stock_Deposits", v[704], 0, 1);  // Set lag value
        v[705] += v[704];
    }
    }  // End CYCLE(cur2, "CLASSES")

    LOG("\n[Phase C] Initialized Household_Stock_Deposits (Skill/Profit-Share Weighted):");
    LOG("\n  Total deposits to distribute: %.4f", v[700]);
    LOG("\n  Capitalist share: %.2f%%, Worker share: %.2f%%", v[701]*100, v[702]*100);
    LOG("\n  Worker skill sum: %.4f", v[96]);
    LOG("\n  Total household deposits: %.4f", v[705]);

    // =========================================================================
    // Stage 5.3: Initialize Household Loan Parameters and HOUSEHOLD_LOANS
    // =========================================================================
    v[800] = VS(country, "household_initial_max_debt_rate");

    CYCLE(cur2, "CLASSES")
    {
    CYCLES(cur2, cur, "HOUSEHOLD")
    {
        // Initialize max debt rate from country-level parameter
        WRITES(cur, "Household_Max_Debt_Rate", v[800]);
        WRITELLS(cur, "Household_Max_Debt_Rate", v[800], 0, 1);

        // Initialize debt rate to 0 (no initial debt)
        WRITES(cur, "Household_Debt_Rate", 0);
        WRITELLS(cur, "Household_Debt_Rate", 0, 0, 1);

        // Initialize stock of loans to 0 (no lag needed - computed from HOUSEHOLD_LOANS)
        WRITES(cur, "Household_Stock_Loans", 0);

        // Initialize dummy HOUSEHOLD_LOANS object (all params = 0)
        cur3 = SEARCHS(cur, "HOUSEHOLD_LOANS");
        if(cur3 != NULL)
        {
            WRITES(cur3, "household_loan_total_amount", 0);
            WRITES(cur3, "household_loan_interest_rate", 0);
            WRITES(cur3, "household_loan_fixed_amortization", 0);
        }
    }
    }  // End CYCLE(cur2, "CLASSES")

    LOG("\n[Stage 5.3] Initialized Household Loan Parameters");
    LOG("\n  Initial max debt rate: %.4f", v[800]);

    // =========================================================================
    // Stage 5.4: Initialize Household Financial Assets
    // =========================================================================
    // Start with initial_valuation_ratio × Capital_Stock distributed by profit_share
    v[850] = VS(country, "Country_Capital_Stock");
    v[851] = VS(country, "initial_valuation_ratio");
    v[852] = v[850] * v[851];  // Total initial financial wealth

    CYCLE(cur2, "CLASSES")
    {
    CYCLES(cur2, cur, "HOUSEHOLD")
    {
        v[853] = VS(cur, "household_type");
        if(v[853] == 1)  // Capitalist
        {
            v[854] = VS(cur, "household_profit_share");
            v[855] = v[852] * v[854];  // Distribute by profit share
            WRITELLS(cur, "Household_Financial_Assets", v[855], t, 1);
        }
        else  // Worker
        {
            WRITELLS(cur, "Household_Financial_Assets", 0, t, 1);
        }
    }
    }  // End CYCLE(cur2, "CLASSES")
    LOG("\n[Stage 5.4] Financial Assets initialized");
    LOG("\n  Total financial wealth: %.2f", v[852]);
    LOG("\n  Initial valuation ratio: %.2f", v[851]);

    // =========================================================================
    // Stage 7: Initialize Wealth Tax Variables
    // =========================================================================
    // Read wealth tax parameters from COUNTRY level
    v[700] = VS(country, "wealth_tax_rate");       // Default 0.02 (2%)
    v[701] = VS(country, "wealth_tax_threshold");  // Default 7650
    v[702] = VS(country, "switch_class_tax_structure");

    // Initialize wealth tax variables for all households
    CYCLE(cur2, "CLASSES")
    {
    CYCLES(cur2, cur, "HOUSEHOLD")
    {
        // All wealth tax variables start at 0
        WRITES(cur, "Household_Wealth_Tax_Owed", 0);
        WRITES(cur, "Household_Wealth_Tax_Payment", 0);
        WRITES(cur, "Household_Wealth_Tax_From_Deposits", 0);
        WRITES(cur, "Household_Wealth_Tax_From_Assets", 0);
        WRITES(cur, "Household_Wealth_Tax_From_Borrowing", 0);
        WRITES(cur, "Household_Wealth_Tax_From_Buffer", 0);
    }
    }  // End CYCLE(cur2, "CLASSES")

    LOG("\n[Stage 7] Wealth Tax Parameters initialized");
    LOG("\n  Wealth tax rate: %.4f", v[700]);
    LOG("\n  Wealth tax threshold: %.2f", v[701]);
    LOG("\n  Tax structure switch: %.0f (5+ = wealth tax active)", v[702]);

    // =========================================================================
    // Stage 9: Initialize Tax Evasion & Capital Flight Variables (MINIMAL SET)
    // =========================================================================
    // Read evasion parameters from appropriate objects
    // NOTE: Uses existing external_interest_rate as offshore rate (tax havens pay world rate)
    v[710] = VS(external, "external_interest_rate");          // World rate = offshore rate
    v[711] = VS(government, "audit_probability");             // Default 0.05 (GOVERNMENT - enforcement)
    v[712] = VS(government, "penalty_rate");                  // Default 2.0 (GOVERNMENT - enforcement)
    v[713] = VS(country, "country_avg_propensity_evade");     // Default 0.5 (COUNTRY - population baseline)
    v[714] = VS(government, "enforcement_sensitivity");       // Default 0 (fixed enforcement)

    // Initialize dynamic audit probability (= base rate at t=0)
    WRITES(government, "Government_Dynamic_Audit_Probability", v[711]);

    // Initialize evasion variables for all households (10 variables)
    CYCLE(cur2, "CLASSES")
    {
    CYCLES(cur2, cur, "HOUSEHOLD")
    {
        // Capital Flight: 4 variables (1 core + 2 dummies + 1 derived)
        WRITES(cur, "Household_Deposits_Offshore", 0);
        WRITELLS(cur, "Household_Deposits_Offshore", 0, 0, 1);
        WRITES(cur, "Household_Decision_Flight", 0);           // DUMMY
        WRITES(cur, "Household_Offshore_Interest", 0);         // DUMMY
        WRITES(cur, "Household_Deposits_Domestic", VS(cur, "Household_Stock_Deposits"));
        WRITELLS(cur, "Household_Deposits_Domestic", VS(cur, "Household_Stock_Deposits"), 0, 1);

        // Asset Evasion: 4 variables (1 core + 2 dummies + 1 enforcement)
        WRITES(cur, "Household_Assets_Undeclared", 0);
        WRITELLS(cur, "Household_Assets_Undeclared", 0, 0, 1); // Lag needed for Wealth_Tax_Owed
        WRITES(cur, "Household_Decision_Evasion", 0);          // DUMMY (hide fraction)
        WRITES(cur, "Household_Assets_Declared", VLS(cur, "Household_Financial_Assets", 1));  // DUMMY
        WRITES(cur, "Household_Is_Audited", 0);
        WRITES(cur, "Household_Asset_Penalty", 0);

        // Repatriation: 1 variable (consolidated)
        WRITES(cur, "Household_Repatriated_Deposits", 0);
    }
    }  // End CYCLE(cur2, "CLASSES")

    LOG("\n[Stage 9] Tax Evasion Variables initialized (10 vars/household)");
    LOG("\n  Offshore rate (= external_interest_rate): %.4f", v[710]);
    LOG("\n  Audit probability (base): %.4f", v[711]);
    LOG("\n  Penalty rate: %.4f", v[712]);
    LOG("\n  Country avg propensity to evade: %.4f", v[713]);
    LOG("\n  Enforcement sensitivity: %.2f (0 = fixed, >0 = dynamic)", v[714]);
}

v[271]=WHTAVE("sector_initial_productivity", "sector_initial_demand")/SUM("sector_initial_demand");
v[272]=WHTAVE("sector_desired_degree_capacity_utilization", "sector_initial_demand")/SUM("sector_initial_demand");
	
//WRITTING COUNTRY LAGGED VALUES  
WRITELLS(country, "Country_Debt_Rate_Firms", v[220], 0, 1);
WRITELLS(country, "Country_Idle_Capacity", 1-v[272], 0, 1);
WRITELLS(country, "Country_Avg_Productivity", v[271], 0, 1);
WRITELLS(country, "Country_Avg_Productivity", v[271], 0, 2);
WRITELLS(country, "Country_Annual_CPI_Inflation", v[70], 0, 1);
for(i=1;i<=v[0]+1;i++)		WRITELLS(country, "Country_Price_Index", v[270], 0, i);
for(i=1;i<=v[0]+1;i++)		WRITELLS(country, "Country_Consumer_Price_Index", v[13], 0, i);
for(i=1;i<=2*v[0]+1;i++)	WRITELLS(country, "Country_GDP", v[100], 0, i);
for(i=1;i<=2*v[0]+1;i++)	WRITELLS(country, "Country_Real_GDP", v[100]/v[270], 0, i);
for(i=1;i<=v[0];i++)		WRITELLS(country, "Country_Capital_Goods_Price", v[23], 0, i);

// =========================================================================
// INITIALIZATION DIAGNOSTICS - Comprehensive snapshot of initial model state
// =========================================================================
log_initialization_diagnostics();

PARAMETER
RESULT(0)