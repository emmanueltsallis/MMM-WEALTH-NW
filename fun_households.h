/*******************************************************************************
 * fun_households.h - Household Behavior Module
 *
 * Stage 5.2: MARKOV Employment Transitions with Gap-Driven Probabilities
 *
 * REFERENCE MODEL:
 *   Target implementation: ref_current_model/fun_households.h
 *   (MMM_v.3.6_Households_Wealth - fully disaggregated household model)
 *   See variable_mapping.txt for Class -> Household variable mapping
 *
 * STAGE HISTORY:
 * - Stage 1: COMPLETE - Empty shell wrapping fun_classes.h
 * - Stage 2: COMPLETE - HOUSEHOLDS container with identity mapping
 * - Stage 3: COMPLETE - Type distinction (workers vs capitalists)
 * - Stage 4: COMPLETE - Heterogeneity, consumption ratchet, budget constraint
 * - Stage 4.7: COMPLETE - The Switch (class→household consumption)
 * - Stage 5.1: COMPLETE - Q-exponential profit distribution
 * - Stage 5.2: CURRENT - MARKOV employment transitions
 *
 * STAGE 5.2 DESIGN (Gap-Driven Employment):
 * - Workers have Employment_State (0=unemployed, 1=employed)
 * - Transitions based on capacity utilization gap (stabilizer)
 * - P_hire = max(0, gap) × hiring_speed × skill_boost
 * - P_fire = max(0, -gap) × firing_speed / skill_protection
 * - Unemployed workers receive benefits (government_benefit_rate)
 * - Parameters: employment_hiring_speed (0.5), employment_firing_speed (0.2)
 *
 * KEY INVARIANTS:
 * - SUM(Household_Wage_Income) = Country_Total_Wages (employed only)
 * - SUM(Household_Profit_Income) = Country_Total_Profits
 * - Income = Wages + Profits + Benefits
 *
 ******************************************************************************/

#ifndef FUN_HOUSEHOLDS_H
#define FUN_HOUSEHOLDS_H

// NOTE: fun_classes.h removed as part of CLASS system removal (Stage 5.5)
// All CLASS-based equations have been replaced with HOUSEHOLD equivalents


/******************************************************************************
 * HOUSEHOLD IDENTIFICATION
 *
 * Requirements in LSD Browser:
 * - HOUSEHOLDS object under COUNTRY
 * - HOUSEHOLD instances under HOUSEHOLDS (e.g., 30 total)
 * - Parameter: household_id (unique identifier)
 * - Parameter: household_type (0=worker, 1=capitalist)
 *****************************************************************************/


EQUATION("Household_Id")
/*
Returns this household's unique identifier.
Simple pass-through to make household_id parameter visible in LSD Browser.
*/
RESULT(V("household_id"))


/******************************************************************************
 * STAGE 3: TYPE DISTINCTION EQUATIONS
 *
 * These equations distinguish workers from capitalists.
 * - Workers (household_type=0): Receive wage income
 * - Capitalists (household_type=1): Receive profit income
 *
 * HOMOGENEOUS ASSUMPTION:
 * - All workers receive equal share of Country_Total_Wages
 * - All capitalists receive equal share of Country_Total_Profits
 * - No skill heterogeneity (skill=1.0 for all)
 * - No employment dynamics (all workers employed)
 *
 * Stage 4+ will introduce heterogeneity.
 *****************************************************************************/


EQUATION("Household_Employment_Status")
/*
Stage 5.2b: MARKOV employment transitions using CAPACITY UTILIZATION signal.
Uses baseline churn + gap amplification for stable secular stagnation dynamics.

Employment State Codes:
- -1 = Capitalist (not in labor force)
-  0 = Unemployed worker
-  1 = Employed (simplified - no sector tracking for wage normalization)

CAPACITY UTILIZATION APPROACH (Post-Keynesian/Kaleckian):
- Signal = Country_Capacity_Utilization - target_utilization
- Baseline churn ensures job turnover even at equilibrium (matching friction)
- Gap amplifies/dampens baseline rates
- Supports stable low-level equilibrium (secular stagnation)

FORMULA:
- P_hire = base_hire × (1 + hiring_speed × max(0, gap)) × skill_boost
- P_fire = base_fire × (1 + firing_speed × max(0, -gap)) / skill_protect

SKILL MODULATION:
- High-skill workers are more likely to be hired (skill attracts, capped at 2×)
- High-skill workers are less likely to be fired (skill protects, floor at 0.5)
*/
v[0] = V("household_type");
if(v[0] == 1)  // Capitalist - not in labor market
{
    v[10] = -1;
}
else  // Worker
{
    // Get previous state
    v[2] = CURRENT;
    if(v[2] < 0) v[2] = 1;  // Initialize: worker starts employed

    // CAPACITY UTILIZATION SIGNAL (the core Kaleckian variable)
    v[3] = VS(country, "Country_Capacity_Utilization");
    v[4] = V("employment_target_utilization");
    if(v[4] <= 0) v[4] = 0.85;  // Default target
    v[5] = v[3] - v[4];  // Gap: positive = economy hot, negative = slack

    // BASELINE CHURN RATES (ensures job turnover even at equilibrium)
    v[6] = V("employment_base_hire_rate");
    v[7] = V("employment_base_fire_rate");
    if(v[6] <= 0) v[6] = 0.02;  // Default: 2% hiring baseline
    if(v[7] <= 0) v[7] = 0.01;  // Default: 1% firing baseline

    // SPEED PARAMETERS (amplification of baseline by gap)
    v[8] = V("employment_hiring_speed");
    v[9] = V("employment_firing_speed");
    if(v[8] <= 0) v[8] = 0.5;   // Default
    if(v[9] <= 0) v[9] = 0.2;   // Default (slower firing = institutional friction)

    // SKILL EFFECT
    v[11] = V("household_skill");

    if(v[2] == 0)  // Currently UNEMPLOYED → check hiring
    {
        // P_hire = base × (1 + speed × max(0, gap)) × skill_boost
        v[12] = v[6] * (1.0 + v[8] * max(0.0, v[5])) * min(2.0, v[11]);
        v[12] = min(0.50, max(0.0, v[12]));  // Cap at 50%

        if(RND < v[12])
            v[10] = 1;  // Hired
        else
            v[10] = 0;  // Stay unemployed
    }
    else  // Currently EMPLOYED → check firing
    {
        // P_fire = base × (1 + speed × max(0, -gap)) / skill_protect
        v[13] = v[7] * (1.0 + v[9] * max(0.0, -v[5])) / max(0.5, v[11]);
        v[13] = min(0.20, max(0.0, v[13]));  // Cap at 20% (institutional friction)

        if(RND < v[13])
            v[10] = 0;  // Fired
        else
            v[10] = 1;  // Stay employed
    }
}

RESULT(v[10])


EQUATION("Household_Wage_Income")
/*
Stage 5.2b: Wage income using SKILL-NORMALIZED SHARES.
Guarantees identity: SUM(Household_Wage_Income) = Country_Total_Wages

Homogeneous mode (switch=0): Equal wages for all employed workers
Heterogeneous mode (switch=1): Skill-weighted wage shares

FORMULA (Heterogeneous):
  Wage[i] = Country_Total_Wages × (skill[i] / Country_Total_Employed_Skill)

This ensures identity by construction: SUM(skill/Total_Skill) = 1

Income sources:
- Capitalists: 0 (no wage income)
- Employed workers: Skill-normalized share of total wages
- Unemployed workers: 0 (get benefits instead via Household_Unemployment_Benefits)
*/
v[0] = V("household_type");

if(v[0] == 1)  // Capitalist - no wage income
{
    v[5] = 0;
}
else  // Worker
{
    v[1] = V("Household_Employment_Status");

    if(v[1] > 0)  // Employed
    {
        v[2] = VS(country, "Country_Total_Wages");
        v[3] = VS(country, "switch_household_heterogeneity");

        if(v[3] == 0)  // Homogeneous: equal wages for all employed
        {
            v[4] = VS(country, "Country_Total_Employment");
            v[5] = (v[4] > 0) ? v[2] / v[4] : 0;
        }
        else  // Heterogeneous: skill-weighted share
        {
            v[4] = VS(country, "Country_Total_Employed_Skill");
            v[6] = V("household_skill");
            v[5] = (v[4] > 0) ? v[2] * (v[6] / v[4]) : 0;
        }
    }
    else  // Unemployed - no wage income
    {
        v[5] = 0;
    }
}

RESULT(v[5])


EQUATION("Household_Profit_Income")
/*
Stage 5.1: Profit income for capitalist households using Q-EXPONENTIAL DISTRIBUTION.

- Workers: 0 (no profit income)
- Capitalists: Get share proportional to household_profit_share (q-exponential)

The profit share is initialized in fun_init_2.h using q-exponential distribution:
- Homogeneous mode (switch=0): Equal shares (1/N_capitalists each)
- Heterogeneous mode (switch=1): Q-exponential draws, normalized to sum=1

Note: This equation is for accounting purposes. Consumption decisions use
lagged income variables (Household_Avg_Real_Income) to avoid simultaneity.

INVARIANT: SUM(Household_Profit_Income) = Country_Total_Profits
*/
v[0] = V("household_type");

if(v[0] == 0)  // Worker
{
    v[3] = 0;
}
else
{
    // Capitalist: get profit share × total profits
    // Note: Circularity avoided because consumption chain uses:
    // - Household_Avg_Real_Income (lagged) for percentile
    // - Country_Median_Household_Income (lagged) for comparison
    // - Bridge equations for budget constraints
    v[1] = VS(country, "Country_Total_Profits");
    v[2] = V("household_profit_share");  // Stage 5.1: Q-exponential share
    v[3] = v[1] * v[2];
}

RESULT(v[3])


EQUATION("Household_Unemployment_Benefits")
/*
Stage 5.2: Unemployment benefits for unemployed workers.
SFC-CORRECT: Benefits come from Government_Effective_Unemployment_Benefits,
distributed equally among all unemployed worker households.

- Capitalists: 0 (not eligible)
- Employed workers: 0 (no benefits while employed)
- Unemployed workers: Government_Effective_Unemployment_Benefits / Country_Unemployed_Households

This maintains Stock-Flow Consistency: money flows Government → Households.
Government budget constraint limits total benefits payable.

Reference: target model fun_households.h lines 935-951
*/
v[0] = V("household_type");
if(v[0] == 1)  // Capitalist - no benefits
{
    v[4] = 0;
}
else  // Worker
{
    v[1] = V("Household_Employment_Status");
    if(v[1] == 1)  // Employed - no benefits
    {
        v[4] = 0;
    }
    else  // Unemployed - receives share of government benefits
    {
        // SFC-correct: Get total benefits from government budget
        v[2] = VS(government, "Government_Effective_Unemployment_Benefits");
        // Get count of unemployed workers
        v[3] = VS(country, "Country_Unemployed_Households");
        // Distribute equally among all unemployed
        v[4] = (v[3] > 0) ? v[2] / v[3] : 0;
    }
}

RESULT(max(0, v[4]))


/******************************************************************************
 * STAGE 4.2: INCOME AND AVERAGING EQUATIONS
 *
 * Simplified versions for Stage 4 - no taxes/benefits yet.
 * These prepare for consumption behavior (Duesenberry) in later stages.
 *
 * Reference: ref_current_model/fun_households.h (lines 918-984)
 * Reference: original UFRJ fun_classes.h (lines 4-8)
 *****************************************************************************/


EQUATION("Household_Nominal_Gross_Income")
/*
Stage 5.2: Nominal GROSS income - TAXABLE income sources only.

Taxable income sources:
1. Wage income (employed workers only)
2. Profit income (capitalists only, q-exponential shares)
3. Deposit interest (LAGGED - earned on last period's deposits)

TAX-FREE sources (added directly to Disposable Income):
- Unemployment benefits (social safety net)
- Wealth transfers (Stage 7.5 redistribution)

Uses LAGGED deposit return to avoid circular dependency.
*/
// Current wage income (employed workers)
v[0] = V("Household_Wage_Income");

// Current profit income (capitalists)
v[1] = V("Household_Profit_Income");

// Lagged deposit interest (earned on last period's deposits)
v[2] = VL("Household_Deposits_Return", 1);

// Gross (taxable) income
v[3] = v[0] + v[1] + v[2];

RESULT(max(0, v[3]))


EQUATION("Household_Income_Taxation")
/*
Household taxation based on tax structure switch.
Mirrors Class_Taxation logic but uses household-specific income sources.

switch_class_tax_structure (reusing class switch for consistency):
0 --> No Tax
1 --> Only Wages
2 --> Only Profits
3 --> Wages and Profits
4 --> Wages, Profits and Interest
*/
v[0] = V("Household_Wage_Income");            // Worker wage income (0 for capitalists)
v[1] = V("Household_Profit_Income");          // Capitalist profit income (0 for workers)
v[2] = V("Household_Deposits_Return");        // Interest income

v[3] = VS(country, "switch_class_tax_structure");  // Tax structure
v[4] = VS(country, "household_income_tax_rate");   // Income tax rate (Stage 5.5: moved to COUNTRY level)

// Compute tax based on structure
if(v[3] == 0)
    v[5] = 0;                                 // No tax
else if(v[3] == 1)
    v[5] = v[0] * v[4];                       // Only wages
else if(v[3] == 2)
    v[5] = v[1] * v[4];                       // Only profits
else if(v[3] == 3)
    v[5] = (v[0] + v[1]) * v[4];              // Wages and profits
else if(v[3] == 4)
    v[5] = (v[0] + v[1] + v[2]) * v[4];       // Wages, profits, interest
else
    v[5] = (v[0] + v[1]) * v[4];              // Default: wages and profits

RESULT(max(0, v[5]))


EQUATION("Household_Nominal_Disposable_Income")
/*
Stage 5.2: Nominal DISPOSABLE income = Gross - Taxes + Tax-Free Income.

Tax-free additions:
- Unemployment benefits (social safety net)
- Wealth transfers (Stage 7.5 redistribution)

Note: Consumption decisions use LAGGED disposable income to break
circular dependency (consumption → demand → production → income).
*/
v[0] = V("Household_Nominal_Gross_Income");
v[1] = V("Household_Income_Taxation");

// Tax-free income sources (not in Gross, not taxed)
v[2] = V("Household_Unemployment_Benefits");
v[3] = V("Household_Transfer_Received");

// Disposable = (Gross - Taxes) + Tax-Free
v[4] = v[0] - v[1] + v[2] + v[3];

RESULT(max(0, v[4]))


EQUATION("Household_Real_Disposable_Income")
/*
Stage 4.2: Real disposable income = Nominal / CPI.

Reference: ref_current_model/fun_households.h (lines 976-984)
*/
v[0] = V("Household_Nominal_Disposable_Income");
v[1] = VS(country, "Country_Consumer_Price_Index");

// Guard against division by zero
v[2] = (v[1] > 0) ? v[0] / v[1] : 0;

RESULT(v[2])


EQUATION("Household_Avg_Real_Income")
/*
Stage 4.2: Average real disposable income over annual_frequency periods.

Used for Duesenberry consumption (relative income effect).
Special handling at t=0,1 to avoid circular dependency with lagged values.

Reference: ref_current_model/fun_households.h (lines 3-16)
Reference: original UFRJ fun_classes.h (lines 4-5)
*/
v[1] = V("annual_frequency");

if(t == 0)
    v[0] = 0;  // Initialization - will be set by WRITELLS if needed
else if(t == 1)
    v[0] = VL("Household_Real_Disposable_Income", 1);  // Use t=0 value to break circular dependency
else
    v[0] = LAG_AVE(p, "Household_Real_Disposable_Income", v[1], 1);  // Average of lagged values

RESULT(v[0])


EQUATION("Household_Avg_Nominal_Income")
/*
Stage 4.2: Average nominal disposable income over annual_frequency periods.

Reference: ref_current_model/fun_households.h (lines 18-31)
Reference: original UFRJ fun_classes.h (lines 7-8)
*/
v[1] = V("annual_frequency");

if(t == 0)
    v[0] = 0;  // Initialization
else if(t == 1)
    v[0] = VL("Household_Nominal_Disposable_Income", 1);  // Use t=0 value
else
    v[0] = LAG_AVE(p, "Household_Nominal_Disposable_Income", v[1], 1);  // Average of lagged values

RESULT(v[0])


/******************************************************************************
 * STAGE 4.3: CONSUMPTION RATCHET MODULE (Duesenberry Effect)
 *
 * Post-Keynesian/Kaleckian approach with habit formation:
 * - Base propensity determined by income percentile (poor spend more)
 * - Ratchet effect: households resist cutting consumption when income falls
 * - Asymmetric adjustment: habits inflate fast, deflate slow (loss aversion)
 *
 * Three modes via switch_household_propensity_hysteresis:
 * - Mode 0: No hysteresis (static propensity based on income percentile)
 * - Mode 1: Memory-fading habits (temporary ratchet, habits eventually adjust)
 * - Mode 2: True hysteresis (λ⁻=0, reference income never decreases)
 *
 * Key equations:
 * - Reference Income: R_{t+1} = R_t + λ(Y_t - R_t) with λ⁺ or λ⁻
 * - Income Percentile: x = (Y/median) / (1 + Y/median)
 * - Base MPC: c̄ = 1 / (1 + e^(k(ax-b)))^(1/ν)
 * - Final MPC: MPC = c̄ + β × max(0, 1 - Y/R)
 *
 * Reference: Plan file distributed-jumping-sedgewick.md
 *****************************************************************************/


EQUATION("Household_Reference_Income")
/*
Stage 4.3: Reference income (habit stock) for consumption ratchet.

R represents the lifestyle the household is "used to":
- When Y > R: reference rises via λ⁺ (habits inflate)
- When Y ≤ R: reference falls via λ⁻ (habits deflate slowly)

Modes (switch_household_propensity_hysteresis):
- Mode 0: R = Y (no habit memory)
- Mode 1: Asymmetric adjustment (R evolves with λ⁺, λ⁻)
- Mode 2: True hysteresis (λ⁻ = 0, R never decreases)

Initialize: R_0 = Y_0 (set in fun_init_2.h via WRITELLS)

Uses AVERAGE income to:
1. Avoid circular dependency (current income → profits → consumption)
2. Habit adjustment responds to sustained income changes, not one-period shocks
*/
v[0] = VS(country, "switch_household_propensity_hysteresis");
v[1] = V("Household_Avg_Real_Income");          // Average income Y (safe from circularity)
v[2] = VL("Household_Reference_Income", 1);     // Previous reference R

// Mode 0: No hysteresis - reference equals current income
if(v[0] == 0)
{
    v[6] = v[1];
}
else
{
    // Mode 1 or 2: Asymmetric adjustment
    v[3] = V("household_habit_inflation");   // λ⁺
    v[4] = V("household_habit_deflation");   // λ⁻ (0 in Mode 2)

    if(v[1] > v[2])
    {
        // Income above reference: inflate habits
        // R_{t+1} = R_t + λ⁺(Y_t - R_t)
        v[5] = v[3];  // Use λ⁺
    }
    else
    {
        // Income at or below reference: deflate habits (slowly)
        // R_{t+1} = R_t + λ⁻(Y_t - R_t)
        v[5] = v[4];  // Use λ⁻
    }

    // Reference income evolution
    v[6] = v[2] + v[5] * (v[1] - v[2]);
}

// Ensure positive
v[7] = max(0.01, v[6]);

RESULT(v[7])


EQUATION("Household_Income_Percentile")
/*
Stage 4.3: Household position in income distribution [0, 1].

Uses median-based proxy for efficiency:
  income_ratio = Y / median_income
  percentile = income_ratio / (1 + income_ratio)

Properties:
- Maps [0, ∞) to [0, 1)
- At median income: percentile = 0.5
- Poor (Y << median): percentile → 0
- Rich (Y >> median): percentile → 1

Uses AVERAGE income because:
1. Social position reflects sustained income, not single-period fluctuation
2. Consistent with Duesenberry relative income theory
3. Household_Avg_Real_Income uses LAG_AVE internally - avoids circular dependency
4. Target model uses same approach (VL(Avg_Nominal_Income, 1))
*/
v[0] = V("Household_Avg_Real_Income");  // Average income - stable, safe from circularity
v[1] = VS(country, "Country_Median_Household_Income");

// Income ratio relative to median
v[2] = (v[1] > 0.01) ? v[0] / v[1] : 1.0;

// Transform to [0, 1] range: x / (1 + x)
v[3] = v[2] / (1 + v[2]);

RESULT(v[3])


EQUATION("Household_Propensity_to_Spend")
/*
Stage 4.3: Propensity to spend (MPC) with Consumption Ratchet effect.

Final MPC = Base_MPC + Ratchet_Boost

Base MPC (Generalized Logistic based on income percentile):
  c̄ = 1 / (1 + e^(k(ax-b)))^(1/ν)

Where:
  x = income_percentile [0, 1]
  k = steepness, a = slope, b = shift, ν = asymmetry

Ratchet Boost (Duesenberry effect):
  boost = β × max(0, 1 - Y/R)

Where:
  β = household_habit_persistence (how stubborn about lifestyle)
  Y = current income, R = reference income

When Y < R (income below habits): boost > 0 (fight to maintain lifestyle)
When Y ≥ R (income at/above habits): boost = 0 (no ratchet needed)

Reference: Plan file distributed-jumping-sedgewick.md
*/
// Get hysteresis mode
v[0] = VS(country, "switch_household_propensity_hysteresis");

// Get base MPC curve parameters (COUNTRY level)
v[1] = VS(country, "household_propensity_steepness");   // k
v[2] = VS(country, "household_propensity_slope");       // a
v[3] = VS(country, "household_propensity_shift");       // b
v[4] = VS(country, "household_propensity_asymmetry");   // ν

// Get household position
v[5] = V("Household_Income_Percentile");  // x ∈ [0, 1]

// Calculate base MPC using generalized logistic
// c̄ = 1 / (1 + e^(k(ax-b)))^(1/ν)
v[6] = v[1] * (v[2] * v[5] - v[3]);  // k(ax - b)
v[7] = exp(v[6]);                      // e^(k(ax-b))
v[8] = 1.0 + v[7];                     // 1 + e^(...)

// Handle asymmetry parameter (avoid division by zero)
v[9] = (fabs(v[4]) > 0.001) ? 1.0 / v[4] : 1.0;
v[10] = pow(v[8], v[9]);               // (1 + e^(...))^(1/ν)
v[11] = 1.0 / v[10];                   // Base MPC

// Mode 0: Just base MPC, no ratchet
if(v[0] == 0)
{
    v[16] = v[11];
}
else
{
    // Mode 1 or 2: Add ratchet effect
    // Use AVERAGE income for Y (not current) to:
    // 1. Avoid circular dependency (current income → profits → consumption)
    // 2. Lifestyle comparison should reflect sustained income, not one-period fluctuation
    v[12] = V("Household_Avg_Real_Income");          // Y (average, safe)
    v[13] = V("Household_Reference_Income");         // R (habit stock)
    v[14] = V("household_habit_persistence");        // β

    // Ratchet boost: β × max(0, 1 - Y/R)
    v[15] = (v[13] > 0.01) ? 1.0 - v[12] / v[13] : 0;
    v[15] = max(0, v[15]);  // Only positive (when Y < R)

    // Final MPC = base + ratchet boost
    v[16] = v[11] + v[14] * v[15];
}

// Propensity can exceed 1.0 (want to spend more than income)
// Budget constraint in Stage 4.6 will limit actual spending
v[17] = max(0, v[16]);

RESULT(v[17])


/******************************************************************************
 * STAGE 4.4a: AUTONOMOUS CONSUMPTION
 *
 * Base consumption that evolves with quality growth, independent of income.
 * Heterogeneity via household_autonomous_consumption_adjustment parameter.
 *
 * Reference: ref_current_model/fun_households.h (lines 35-60)
 *****************************************************************************/


EQUATION("Household_Real_Autonomous_Consumption")
/*
Stage 4.4a: Autonomous consumption evolves with quality growth.

This represents a minimum consumption floor that grows with product quality
improvements in the consumption sector. Heterogeneity is introduced through
the household-specific adjustment parameter assigned at initialization.

Key features:
- Adjusts annually (when t mod annual_frequency == 0)
- Grows proportionally with consumption sector quality growth
- Heterogeneous via household_autonomous_consumption_adjustment

Reference: ref_current_model/fun_households.h (lines 35-60)
*/
v[0] = CURRENT;                   // Previous period's autonomous consumption
v[1] = V("annual_frequency");     // Adjustment frequency
v[2] = fmod((double) t, v[1]);    // Check if adjustment period

if(v[2] == 0)  // Annual adjustment period
{
    v[3] = LAG_GROWTH(consumption, "Sector_Avg_Quality", v[1], 1);

    // Household-specific adjustment (heterogeneous, set at initialization)
    v[4] = V("household_autonomous_consumption_adjustment");

    // Apply adjustment: autonomous_consumption * (1 + adjustment * quality_growth)
    v[5] = v[0] * (1 + v[4] * v[3]);

    // Ensure non-negative
    v[6] = max(0, v[5]);
}
else
{
    v[6] = v[0];  // Not adjustment period, keep previous value
}

RESULT(v[6])


/******************************************************************************
 * STAGE 4.4b: IMPORT SHARE EQUATION
 *
 * Determines what fraction of consumption is imported vs domestic.
 * Uses budget-based elasticity: poor households are more price-sensitive.
 *****************************************************************************/


EQUATION("Household_Imports_Share")
/*
Stage 4.4b: Import share with budget-based price elasticity.

Post-Keynesian approach: Poor households are more price-sensitive than rich.
- Innate propensity: household_import_propensity (heterogeneous, set at init)
- Price ratio: domestic_price / (foreign_price * exchange_rate)
- Budget-based elasticity: 2.0 for poor → 0.5 for rich (endogenous)

Budget calculation:
- discretionary_ratio = (avg_income - autonomous_consumption) / avg_income
- Poor households (low discretionary ratio) → elasticity ≈ 2.0
- Rich households (high discretionary ratio) → elasticity ≈ 0.5

This removes the need for calibrated elasticity parameters - behavior emerges
from household's budget position relative to their subsistence needs.

Reference: Original UFRJ Class_Imports_Share logic with budget-based extension
*/
// 1. Innate propensity to import (heterogeneous)
v[0] = V("household_import_propensity");

// 2. Price ratio: domestic / (foreign * exchange_rate)
v[1] = VLS(consumption, "Sector_Avg_Price", 1);       // Lagged domestic price
v[2] = VS(consumption, "Sector_External_Price");      // Foreign price
v[3] = VS(external, "Country_Exchange_Rate");         // Exchange rate
v[4] = (v[2] * v[3] > 0.01) ? v[1] / (v[2] * v[3]) : 1.0;

// 3. Budget-based elasticity calculation
v[5] = V("Household_Avg_Real_Income");                // Average income
v[6] = V("Household_Real_Autonomous_Consumption");    // Subsistence floor

// Discretionary ratio: how much above subsistence (0=at subsistence, 1=all discretionary)
v[7] = (v[5] > 0.01) ? max(0, (v[5] - v[6]) / v[5]) : 0;

// Elasticity function: poor (v[7]≈0) → 2.0, rich (v[7]≈1) → 0.5
// Linear interpolation: elasticity = 2.0 - 1.5 * discretionary_ratio
v[8] = 2.0 - 1.5 * v[7];
v[9] = max(0.5, min(2.0, v[8]));  // Bound elasticity [0.5, 2.0]

// 4. Final import share = propensity * (price_ratio) ^ elasticity
v[10] = v[0] * pow(v[4], v[9]);

// Bound between 0 and 0.95 (some domestic consumption always)
v[11] = max(0, min(0.95, v[10]));

RESULT(v[11])


/******************************************************************************
 * STAGE 4.5: DESIRED CONSUMPTION EQUATIONS
 *
 * Calculates household desired consumption (domestic and imported).
 * Formula: Domestic = Avg_Income × Propensity × (1 - Import_Share) + Autonomous
 *          Imported = Avg_Income × Propensity × Import_Share
 *
 * Reference: Original UFRJ fun_classes.h (lines 45-63)
 * Reference: ref_current_model/fun_households.h (lines 136-176)
 *****************************************************************************/


EQUATION("Household_Real_Desired_Domestic_Consumption")
/*
Stage 4.5: Real desired domestic consumption.

Formula: Avg_Income × Propensity × (1 - Import_Share) + Autonomous_Consumption

This is the household's desired consumption of domestically-produced goods,
based on their average income, propensity to spend, import preferences,
and autonomous consumption floor.

Reference: Original UFRJ fun_classes.h (lines 45-54)
*/
v[0] = V("Household_Avg_Real_Income");              // Average real income
v[1] = V("Household_Propensity_to_Spend");          // Propensity to spend (type-based)
v[2] = V("Household_Real_Autonomous_Consumption");  // Autonomous consumption floor
v[3] = V("Household_Imports_Share");                // Share going to imports

// Domestic consumption = income-based + autonomous
v[4] = v[0] * v[1] * (1 - v[3]) + v[2];

// Ensure non-negative
v[5] = max(0, v[4]);

RESULT(v[5])


EQUATION("Household_Real_Desired_Imported_Consumption")
/*
Stage 4.5: Real desired imported consumption.

Formula: Avg_Income × Propensity × Import_Share

This is the household's desired consumption of imported goods.
Note: No autonomous component for imports (subsistence needs met domestically).

Reference: Original UFRJ fun_classes.h (lines 57-63)
*/
v[0] = V("Household_Avg_Real_Income");      // Average real income
v[1] = V("Household_Propensity_to_Spend");  // Propensity to spend
v[2] = V("Household_Imports_Share");        // Share going to imports

// Imported consumption = income × propensity × import_share
v[3] = v[0] * v[1] * v[2];

// Ensure non-negative
v[4] = max(0, v[3]);

RESULT(v[4])


/******************************************************************************
 STAGE 4.6: BUDGET CONSTRAINT + FINANCIAL INFRASTRUCTURE

 Full infrastructure in observer mode. Uses bridge equations as inputs
 to maintain identical macro outputs until Stage 4.7 switch.

 GROUP A: Budget Constraint Core (5 equations)
 GROUP B: Financial Stocks (3 equations)
 GROUP C: Savings Flow (2 equations)
 *****************************************************************************/


// =============================================================================
// GROUP A: BUDGET CONSTRAINT CORE
// =============================================================================

EQUATION("Household_Desired_Expenses")
/*
Stage 4.6: Nominal value of desired consumption (domestic + imported).
Uses lagged price to prevent circular dependency.
*/
v[0] = V("Household_Real_Desired_Domestic_Consumption");
v[1] = V("Household_Real_Desired_Imported_Consumption");
v[2] = VLS(consumption, "Sector_Avg_Price", 1);  // Lagged to prevent circular
v[3] = VS(consumption, "Sector_External_Price");
v[4] = VS(external, "Country_Exchange_Rate");

v[5] = v[0]*v[2] + v[1]*v[3]*v[4];
RESULT(max(0, v[5]))


EQUATION("Household_Retained_Deposits")
/*
Stage 4.6: Desired liquidity buffer based on average income.
Households want to keep some deposits as precautionary savings.

household_liquidity_preference: effective value (0 to 1), initialized from
distribution around country baseline (household_avg_liquidity_preference).
*/
v[0] = V("Household_Avg_Nominal_Income");    // Average income
v[1] = V("household_liquidity_preference");  // Effective preference (heterogeneous)

// Desired buffer = income × liquidity_preference
v[2] = v[0] * v[1];

// Cannot retain more than available
v[3] = VL("Household_Stock_Deposits", 1);
v[4] = VL("Household_Nominal_Disposable_Income", 1);
v[5] = v[3] + v[4];  // Total available

RESULT(max(0, min(v[2], v[5])))


EQUATION("Household_Internal_Funds")
/*
Stage 5.3: Available funds BEFORE borrowing.
= Lagged_Deposits + Lagged_Income - Retained_Deposits - Financial_Obligations

NOTE: Uses LAGGED income to break circular dependency:
  Consumption → Internal_Funds → Income → Profits → Sales → Consumption
The deposit equation uses current income (computed AFTER consumption is determined).

Financial obligations (interest + amortization) are deducted before consumption.
This creates the debt service burden that can squeeze consumption.
*/
v[0] = VL("Household_Stock_Deposits", 1);              // Lagged deposits
v[1] = VL("Household_Nominal_Disposable_Income", 1);   // LAGGED income (breaks cycle)
v[2] = V("Household_Retained_Deposits");               // Liquidity buffer
v[3] = V("Household_Financial_Obligations");           // Stage 5.3: Debt service

v[4] = v[0] + v[1] - v[2] - v[3];
RESULT(max(0, v[4]))


EQUATION("Household_Maximum_Expenses")
/*
Stage 5.3: Budget-constrained expenses.
Uses Household_Funds = Internal_Funds + Effective_Loans.
Households can now borrow to fund consumption beyond internal funds.
*/
v[0] = V("Household_Desired_Expenses");
v[1] = V("Household_Funds");  // Stage 5.3: Includes loans
v[2] = min(v[0], v[1]);  // Cannot exceed total funds
RESULT(max(0, v[2]))


EQUATION("Household_Real_Domestic_Consumption_Demand")
/*
Stage 4.6: Final constrained demand. Domestic priority allocation.
This is the variable that replaces Class demand at Stage 4.7.
*/
v[0] = V("Household_Maximum_Expenses");

// Price safeguards
v[1] = max(0.01, VLS(consumption, "Sector_Avg_Price", 1));
v[2] = max(0.01, VS(consumption, "Sector_External_Price"));
v[3] = max(0.01, VS(external, "Country_Exchange_Rate"));

// Desired amounts
v[4] = V("Household_Real_Desired_Domestic_Consumption");
v[5] = V("Household_Real_Desired_Imported_Consumption");

// Nominal values
v[6] = v[4] * v[1];           // Domestic
v[7] = v[5] * v[2] * v[3];    // Imported

// Two-stage allocation: domestic priority
v[8] = min(v[0], v[6]);       // Domestic spending
v[9] = v[8] / v[1];           // Real domestic

// Remaining to imports
v[10] = max(0, v[0] - v[8]);
v[11] = min(v[10], v[7]);
v[12] = v[11] / (v[2] * v[3]);  // Real imported

WRITE("Household_Real_Imported_Consumption_Demand", v[12]);
RESULT(max(0, v[9]))


EQUATION_DUMMY("Household_Real_Imported_Consumption_Demand", "Household_Real_Domestic_Consumption_Demand")


// =============================================================================
// GROUP B: FINANCIAL STOCKS
// =============================================================================

EQUATION("Household_Stock_Deposits")
/*
Stage 5.4: Household deposit stock with SFC fix.

SIMPLIFIED ACCOUNTING using Household_Savings:
- Savings = Income - Expenses - Obligations (already computed)
- Deposits = Lagged + Savings + Loans - Asset_Purchases - Wealth_Tax

Stage 7: Wealth tax flows:
- Tax paid from deposits → outflow
- Tax borrowing → inflow (via Effective_Loans mechanism)

Stock = Lagged + Savings + Loans - Asset_Purchases - Wealth_Tax_Deposits
*/
v[0] = VL("Household_Stock_Deposits", 1);  // Lagged stock
v[1] = V("Household_Savings");              // Income - Expenses - Obligations
v[2] = V("Household_Effective_Loans");      // Loan inflow (includes wealth tax borrowing)
v[3] = V("Household_Asset_Purchases");      // Asset purchases leave deposits (capitalists only)
v[4] = V("Household_Wealth_Tax_From_Deposits");  // Wealth tax outflow

// Stock evolution: Lagged + Savings + Loans - Asset_Purchases - Wealth_Tax
v[5] = v[0] + v[1] + v[2] - v[3] - v[4];

// Safety bounds
v[6] = min(max(0, v[5]), 1e12);
RESULT(v[6])


EQUATION("Household_Stock_Loans")
/*
Stage 5.3: Household loan stock.
ACTIVE MODE: Sum all HOUSEHOLD_LOANS objects.
This replaces the bridge equation with actual household borrowing.
*/
    v[0] = 0;
    if(COUNT("HOUSEHOLD_LOANS") > 0)
    {
        CYCLE(cur, "HOUSEHOLD_LOANS")
        {
            v[1] = VS(cur, "household_loan_total_amount");
            if(v[1] > 0)
                v[0] += v[1];
        }
    }
RESULT(v[0])


EQUATION("Household_Financial_Assets")
/*
Stage 5.4: Financial asset stock held by capitalist households.
Workers hold deposits only (financial assets = 0).

ACCUMULATION CHANNELS:
1. Capital Gains (price appreciation from asset shortage)
2. Asset Purchases (portion of positive savings based on liquidity preference)

KEY: NO wealth effect on consumption. Wealth is TRAPPED.
This demonstrates how financialization causes demand stagnation.

BUY-BORROW-DIE: When savings negative, DON'T sell assets - borrow instead.

Stage 7: WEALTH TAX can force asset liquidation (exception to BBD).
*/
v[0] = V("household_type");

if(v[0] == 0)  // Worker
{
    v[5] = 0;  // Workers don't hold financial assets
}
else  // Capitalist
{
    // 1. Previous stock
    v[1] = VL("Household_Financial_Assets", 1);

    // 2. Capital gains from asset price inflation
    v[2] = VS(financial, "Financial_Asset_Inflation");
    v[3] = v[1] * v[2];

    // 3. Asset purchases (computed once in Household_Asset_Purchases)
    v[4] = V("Household_Asset_Purchases");

    // 4. Stage 7: Wealth tax asset liquidation (forced sale, exception to BBD)
    v[6] = V("Household_Wealth_Tax_From_Assets");

    // Stock evolution: Previous + Gains + Purchases - Tax_Liquidation
    v[5] = v[1] + v[3] + v[4] - v[6];

    // Safety: non-negative
    v[5] = max(0, v[5]);
}

RESULT(v[5])


// =============================================================================
// GROUP C: SAVINGS FLOW
// =============================================================================

EQUATION("Household_Savings")
/*
Stage 5.4: TRUE Free Cash Flow = Income - Consumption - Debt Service.
ACTIVE MODE: Uses actual household variables (not bridges).

This is uncommitted cash available for:
- Capitalists: Asset purchases (portion based on liquidity preference)
- Workers: Buffer savings (typically near zero)

SFC Note: This flow feeds into both deposits (liquid portion) and
financial assets (illiquid portion, capitalists only).
*/
v[0] = V("Household_Nominal_Disposable_Income");  // Actual income
v[1] = V("Household_Effective_Expenses");          // Actual consumption
v[2] = V("Household_Financial_Obligations");       // Principal + Interest

// True Free Cash Flow = Income - Consumption - Debt Service
v[3] = v[0] - v[1] - v[2];
RESULT(v[3])  // Can be negative (dissaving)


EQUATION("Household_Savings_Rate")
/*
Stage 5.4: Savings as fraction of disposable income.
ACTIVE MODE: Uses actual income (not bridge).
*/
v[0] = V("Household_Savings");
v[1] = V("Household_Nominal_Disposable_Income");

v[2] = (v[1] > 0.01) ? v[0] / v[1] : 0;
RESULT(max(-1, min(1, v[2])))  // Bound to [-1, 1]


EQUATION("Household_Asset_Purchases")
/*
Stage 5.4: Capitalist portfolio allocation - portion of savings to financial assets.
Computed once and used by both Stock_Deposits (outflow) and Financial_Assets (inflow).

Workers: 0 (hold deposits only)
Capitalists with positive savings: Savings × (1 - liquidity_preference)
Capitalists with negative savings: 0 (don't sell assets, borrow instead - BBD)

household_liquidity_preference: effective value (0 to 1), determines how much
of savings stays liquid (deposits) vs goes to financial assets.
*/
v[0] = V("household_type");
if(v[0] == 0)  // Worker
{
    v[3] = 0;
}
else  // Capitalist
{
    v[1] = V("Household_Savings");
    if(v[1] > 0)
    {
        v[2] = V("household_liquidity_preference"); // Determines how much of savings goes to financial assets
        v[3] = v[1] * (1 - v[2]);  // Asset portion = savings × (1 - liq_pref)
    }
    else
        v[3] = 0;  // Buy-Borrow-Die: don't liquidate when savings negative
}
RESULT(max(0, v[3]))


EQUATION("Household_Net_Wealth")
/*
Stage 5.4: Net wealth = Assets - Liabilities.
SFC-correct wealth measure for all households.

Net_Wealth = Stock_Deposits + Financial_Assets - Stock_Loans
           = Liquid Wealth + Financial Wealth - Debt

Can be negative (underwater household).
*/
v[0] = V("Household_Stock_Deposits");     // Liquid wealth (both types)
v[1] = V("Household_Financial_Assets");   // Financial wealth (capitalists only)
v[2] = V("Household_Stock_Loans");        // Debt (both types)

v[3] = v[0] + v[1] - v[2];
RESULT(v[3])


/******************************************************************************
 * STAGE 7: WEALTH TAXATION
 *
 * Two-stage behavioral wealth tax payment:
 * 1. Liquidity preference → deposits contribution
 * 2. Debt capacity → borrow vs liquidate split
 *
 * Activated when switch_class_tax_structure >= 5
 *****************************************************************************/


EQUATION("Household_Wealth_Tax_Owed")
/*
Stage 7+9: Calculate wealth tax liability.
Uses LAGGED values to avoid circular dependency.

Stage 7 (no evasion): Tax = max(0, Net_Wealth[t-1] - threshold) × rate
Stage 9 (with evasion): Tax = max(0, Visible_Wealth[t-1] - threshold) × rate

Visible_Wealth = Domestic_Deposits + Declared_Assets
Hidden wealth (offshore deposits + undeclared assets) is NOT taxed.

Only active when switch_class_tax_structure >= 5.
*/
v[0] = VS(country, "switch_class_tax_structure");
if(v[0] < 5)
{
    v[10] = 0;
}
else
{
    // Get lagged deposit and asset stocks
    v[1] = VL("Household_Stock_Deposits", 1);        // Total deposits (lagged)
    v[2] = VL("Household_Financial_Assets", 1);      // Total assets (lagged)
    v[3] = VL("Household_Stock_Loans", 1);           // Debt (lagged)

    // Stage 9: Get lagged hidden wealth (offshore + undeclared)
    v[4] = VL("Household_Deposits_Offshore", 1);     // Offshore deposits (lagged)
    v[5] = VL("Household_Assets_Undeclared", 1);     // Undeclared assets (lagged)

    // Visible wealth = (total deposits - offshore) + (total assets - undeclared)
    v[6] = (v[1] - v[4]) + (v[2] - v[5]);           // Visible wealth (what govt sees)

    // Tax parameters
    v[7] = VS(country, "wealth_tax_threshold");      // Exemption threshold
    v[8] = VS(country, "wealth_tax_rate");           // Tax rate (e.g., 0.03 = 3%)

    // Tax only VISIBLE wealth ABOVE threshold
    v[10] = max(0, (v[6] - v[7]) * v[8]);
}
RESULT(v[10])


EQUATION("Household_Wealth_Tax_Payment")
/*
Stage 7: Four-stage sequential tax payment (Dissertation Eq. 5.50-5.54).

Implements the Buy-Borrow-Die (BBD) pecking order. Each stage is EXHAUSTED
before proceeding to the next:

  STAGE 1 - Excess Deposits (Eq. 5.50):
    Use deposits above precautionary liquidity buffer.

  STAGE 2 - Borrowing (Eq. 5.51):
    Borrow up to debt capacity. Preferred over asset sales
    to avoid capital gains realization (BBD strategy).

  STAGE 3 - Asset Liquidation (Eq. 5.52):
    Sell financial assets only after borrowing exhausted.
    Critical reconnection mechanism: forces financial circuit
    back into productive economy.

  STAGE 4 - Buffer Invasion (Eq. 5.53):
    Invade precautionary liquidity buffer as last resort.

Identity: T_V = T_M + T_L + T_A + T_B (Eq. 5.54)
*/
v[0] = V("Household_Wealth_Tax_Owed");
v[50] = 0;

if(v[0] <= 0)
{
    WRITE("Household_Wealth_Tax_From_Deposits", 0);
    WRITE("Household_Wealth_Tax_From_Borrowing", 0);
    WRITE("Household_Wealth_Tax_From_Assets", 0);
    WRITE("Household_Wealth_Tax_From_Buffer", 0);
}
else
{
    // ========== STAGE 1: EXCESS DEPOSITS (Eq. 5.50) ==========
    v[10] = VL("Household_Stock_Deposits", 1);      // M_{t-1}
    v[11] = V("household_liquidity_preference");     // t_i
    v[12] = v[10] * v[11];                           // Buffer = t_i × M
    v[13] = max(0, v[10] - v[12]);                   // Excess = (1 - t_i) × M

    v[16] = min(v[0], v[13]);                        // T_M: pay from excess
    v[17] = v[0] - v[16];                            // Remainder after Stage 1

    if(v[17] <= 0.001)
    {
        // Tax fully covered by excess deposits
        WRITE("Household_Wealth_Tax_From_Deposits", v[16]);
        WRITE("Household_Wealth_Tax_From_Borrowing", 0);
        WRITE("Household_Wealth_Tax_From_Assets", 0);
        WRITE("Household_Wealth_Tax_From_Buffer", 0);
    }
    else
    {
        // ========== STAGE 2: BORROWING (Eq. 5.51) ==========
        // Monetary capacity mirrors Household_Max_Loans:
        // L_available = Max_Debt_Rate × (Deposits + Income + Collateral) - Loans
        v[20] = V("Household_Max_Debt_Rate");
        v[21] = VL("Household_Stock_Loans", 1);
        v[22] = VL("Household_Nominal_Disposable_Income", 1);
        v[23] = VL("Household_Financial_Assets", 1);
        v[24] = V("household_type");
        v[25] = (v[24] == 1) ? v[23] : 0;               // Capitalists pledge assets
        v[26] = max(0, v[20] * (v[10] + v[22] + v[25]) - v[21]);

        v[27] = min(v[17], v[26]);                       // T_L: borrow up to capacity
        v[28] = v[17] - v[27];                           // Remainder after Stage 2

        // ========== STAGE 3: ASSET LIQUIDATION (Eq. 5.52) ==========
        v[30] = min(v[28], v[23]);                       // T_A: liquidate up to available
        v[31] = v[28] - v[30];                           // Remainder after Stage 3

        // ========== STAGE 4: BUFFER INVASION (Eq. 5.53) ==========
        v[32] = min(v[31], v[12]);                       // T_B: invade buffer
        v[33] = v[31] - v[32];                           // Residual (should be ~0)

        // Any residual: emergency borrowing (tax payment is mandatory)
        v[27] = v[27] + v[33];

        // Store payment sources for stock equations
        // From_Deposits includes buffer invasion (both are deposit withdrawals)
        WRITE("Household_Wealth_Tax_From_Deposits", v[16] + v[32]);
        WRITE("Household_Wealth_Tax_From_Borrowing", v[27]);
        WRITE("Household_Wealth_Tax_From_Assets", v[30]);
        WRITE("Household_Wealth_Tax_From_Buffer", v[32]);
    }
    v[50] = v[0];
}

RESULT(v[50])


EQUATION_DUMMY("Household_Wealth_Tax_From_Deposits", "Household_Wealth_Tax_Payment")
/*
Stage 7: Portion of wealth tax paid from deposits.
Computed in Household_Wealth_Tax_Payment.
*/


EQUATION_DUMMY("Household_Wealth_Tax_From_Assets", "Household_Wealth_Tax_Payment")
/*
Stage 7: Portion of wealth tax paid by liquidating financial assets.
Only capitalists have financial assets; workers always = 0.
Computed in Household_Wealth_Tax_Payment.
*/


EQUATION_DUMMY("Household_Wealth_Tax_From_Borrowing", "Household_Wealth_Tax_Payment")
/*
Stage 7: Portion of wealth tax paid via new borrowing (Stage 2 + emergency).
Includes any residual after all four stages are exhausted.
Computed in Household_Wealth_Tax_Payment.
*/


EQUATION_DUMMY("Household_Wealth_Tax_From_Buffer", "Household_Wealth_Tax_Payment")
/*
Stage 7: Portion of wealth tax paid by invading precautionary liquidity buffer (Stage 4).
Analysis variable — this amount is ALSO included in From_Deposits for SFC.
Nonzero values indicate household financial distress.
Computed in Household_Wealth_Tax_Payment.
*/


/******************************************************************************
 * STAGE 7.5: WEALTH TRANSFER RECEIPT (OPTIMIZED)
 *
 * Equal distribution to bottom X% of households.
 * Eligibility: Pre-computed flag (Household_Transfer_Eligible) set by
 *              Country_Transfer_Desired during its single-pass count.
 * Distribution: Equal share = effective_budget / eligible_count
 *
 * Household_Transfer_Eligible is an EQUATION_DUMMY written by Country_Transfer_Desired.
 *****************************************************************************/


EQUATION("Household_Transfer_Received")
/*
Stage 7.5 OPTIMIZED: Equal transfer for bottom X% of households.

OPTIMIZATION: Reads Household_Transfer_Eligible flag (set by Country_Transfer_Desired).
No threshold comparison needed - eligibility already determined during country-level count.

Distribution: Equal share = effective_budget / eligible_count
Only active when switch_class_tax_structure >= 5.
*/
v[0] = VS(country, "switch_class_tax_structure");
if(v[0] < 5)
{
    v[10] = 0;
}
else
{
    // Read pre-computed eligibility flag (set by Country_Transfer_Desired)
    v[1] = V("Household_Transfer_Eligible");

    if(v[1] > 0)  // Eligible (flag set during country-level count)
    {
        v[3] = VS(country, "Country_Transfer_Eligible");
        v[4] = VS(government, "Government_Effective_Wealth_Transfer");

        if(v[3] > 0 && v[4] > 0)
        {
            // Equal share = budget / eligible_count
            v[10] = v[4] / v[3];
        }
        else
        {
            v[10] = 0;
        }
    }
    else
    {
        v[10] = 0;  // Not eligible
    }
}
RESULT(max(0, v[10]))


EQUATION_DUMMY("Household_Transfer_Eligible", "Country_Transfer_Desired")
/*
Stage 7.5: Eligibility flag for wealth transfer (0 or 1).
Set by Country_Transfer_Desired during its single-pass household loop.
Read by Household_Transfer_Received to determine if this household gets transfer.
*/


/******************************************************************************
 * STAGE 9: TAX EVASION & CAPITAL FLIGHT MODULE
 *
 * Two-channel evasion system for wealth tax:
 * 1. Capital Flight (Deposits): Move deposits offshore - lower returns but INVISIBLE
 * 2. Tax Evasion (Financial Assets): Don't declare assets - normal returns but AUDIT RISK
 *
 * Decision rules:
 * - Flight: propensity × θ > r_domestic - r_offshore
 * - Assets: hide_fraction = propensity × (1 - p × π)
 * - Repatriation: max(liquidity, discretionary)
 *
 * STRUCTURE: 6 core equations + 4 EQUATION_DUMMYs
 *****************************************************************************/


EQUATION("Household_Deposits_Offshore")
/*
Stage 9: Stock of deposits in tax havens (invisible to government).
CONSOLIDATED: Inlines flight decision logic.

Proportional flight with symmetric deterrence (TODO #2):
  flight_fraction = ψ × max(0, 1 - (p×ρ + Δr) / τ_V)

Where:
  p×ρ = expected penalty per unit (enforcement deterrence)
  Δr  = r_domestic - r_offshore (interest cost)
  τ_V = wealth tax rate (benefit of flying)

Consistent with asset evasion formula: both channels use p×ρ/τ_V deterrence.
Flight only occurs when τ_V > p×ρ + Δr (benefit exceeds total costs).

Inflows: capital flight (proportional), offshore interest
Outflows: repatriation

Writes: Household_Decision_Flight, Household_Offshore_Interest
*/
v[0] = VL("Household_Deposits_Offshore", 1);      // Previous offshore stock
v[1] = VL("Household_Deposits_Domestic", 1);      // Previous domestic stock
v[2] = VL("Household_Net_Wealth", 1);             // For liability check
v[3] = VS(country, "switch_class_tax_structure");

// Default: no flight
v[10] = 0;  // flight_fraction [0,1]
v[11] = 0;  // new_flight_amount

// Check if wealth tax active and household is liable
if(v[3] >= 5 && v[2] > VS(country, "wealth_tax_threshold") && v[1] > 0)
{
    v[4] = VS(country, "wealth_tax_rate");                                    // τ_V
    v[5] = VS(centralbank, "Central_Bank_Basic_Interest_Rate");               // r_domestic
    v[6] = VS(external, "external_interest_rate");                            // r_offshore
    v[7] = V("household_propensity_evade");                                   // ψ [0,1]

    // Enforcement deterrence (symmetric with asset evasion)
    v[20] = VS(government, "Government_Dynamic_Audit_Probability");           // p
    v[21] = VS(government, "penalty_rate");                                   // ρ (absolute)
    v[22] = v[20] * v[21];                                                   // p×ρ

    // Total cost per unit = enforcement + interest differential
    v[8] = v[22] + max(0.0, v[5] - v[6]);   // p×ρ + Δr

    // Proportional flight fraction
    if(v[7] > 0 && v[4] > v[8] && v[4] > 0.0001)
    {
        v[10] = v[7] * (1.0 - v[8] / v[4]);  // ψ × (1 - (p×ρ + Δr)/τ_V)
        v[10] = max(0.0, min(1.0, v[10]));
        v[11] = v[1] * v[10];                 // Flight = fraction × domestic deposits
    }
}
WRITE("Household_Decision_Flight", v[10]);

// Offshore interest earned
v[12] = VS(external, "external_interest_rate");  // r_offshore (world rate)
v[13] = v[0] * v[12];
WRITE("Household_Offshore_Interest", v[13]);

// Repatriation (calculated in Household_Repatriated_Deposits)
v[14] = V("Household_Repatriated_Deposits");

// Stock evolution: previous + flight + interest - repatriation
v[15] = v[0] + v[11] + v[13] - v[14];

RESULT(max(0, v[15]))


EQUATION_DUMMY("Household_Decision_Flight", "Household_Deposits_Offshore")
/*
Stage 9: Proportional flight fraction [0,1].
= ψ × (1 - (p×ρ + Δr) / τ_V). Written by Household_Deposits_Offshore.
*/


EQUATION_DUMMY("Household_Offshore_Interest", "Household_Deposits_Offshore")
/*
Stage 9: Interest earned on offshore deposits.
Written by Household_Deposits_Offshore.
*/


EQUATION("Household_Deposits_Domestic")
/*
Stage 9: Deposits visible to tax authority.
= Total deposits - Offshore deposits
*/
v[0] = V("Household_Stock_Deposits");       // Total deposits
v[1] = V("Household_Deposits_Offshore");    // Offshore

RESULT(max(0, v[0] - v[1]))


EQUATION("Household_Assets_Undeclared")
/*
Stage 9: Financial assets hidden from tax authority.
CORE equation that computes undeclared amount and writes hide fraction.

Dissertation formula (Section 5, Eq. 3629):
  h_i = ψ_i × (1 - p×ρ / τ_V)   if τ_V > p×ρ
  h_i = 0                         otherwise

Deterrence is the ratio of expected penalty (p×ρ) to tax owed (τ_V).
When expected penalty >= tax rate, full deterrence occurs (no evasion).
Higher τ_V makes evasion MORE attractive (higher payoff to hiding).

Writes: Household_Decision_Evasion, Household_Assets_Declared
*/
v[0] = VS(country, "switch_class_tax_structure");
v[1] = VL("Household_Net_Wealth", 1);
v[2] = VS(country, "wealth_tax_threshold");
v[3] = V("household_type");
v[4] = VL("Household_Financial_Assets", 1);

// Default: no evasion
v[10] = 0;   // hide_fraction
v[11] = 0;   // undeclared_assets

// Prerequisites: wealth tax active, liable, capitalist, has assets
if(v[0] >= 5 && v[1] > v[2] && v[3] == 1 && v[4] > 0)
{
    // Get enforcement parameters (use dynamic audit rate)
    v[5] = VS(government, "Government_Dynamic_Audit_Probability");  // p (dynamic)
    v[6] = VS(government, "penalty_rate");           // ρ (penalty multiplier)
    v[7] = V("household_propensity_evade");          // ψ [0,1]
    v[8] = VS(country, "wealth_tax_rate");           // τ_V

    // Deterrence = expected penalty / tax rate = (p × ρ) / τ_V
    v[9] = v[5] * v[6];  // p × ρ (expected penalty rate)

    // Only hide if tax rate exceeds expected penalty (evasion is profitable)
    if(v[8] > v[9] && v[8] > 0.0001)
    {
        // Hide fraction = propensity × (1 - deterrence)
        v[12] = v[7] * (1.0 - v[9] / v[8]);
        v[10] = max(0.0, min(1.0, v[12]));

        // Undeclared assets = total × hide_fraction
        v[11] = v[4] * v[10];
    }
}

// Write outputs
WRITE("Household_Decision_Evasion", v[10]);
WRITE("Household_Assets_Declared", max(0.0, v[4] - v[11]));

RESULT(max(0.0, v[11]))


EQUATION_DUMMY("Household_Decision_Evasion", "Household_Assets_Undeclared")
/*
Stage 9: Hide fraction (0 to 1) - fraction of assets not declared.
Written by Household_Assets_Undeclared.
*/


EQUATION_DUMMY("Household_Assets_Declared", "Household_Assets_Undeclared")
/*
Stage 9: Financial assets declared to tax authority.
= Total_Assets - Undeclared_Assets
Written by Household_Assets_Undeclared.
*/


EQUATION("Household_Repatriated_Deposits")
/*
Stage 9: Total repatriation = max(Liquidity, Discretionary).

Liquidity: MUST bring back (covers obligations shortfall)
Discretionary: CHOOSES to bring back (inverse of proportional flight decision)

Flight fraction:      ψ × (1 - (p×ρ + Δr) / τ_V)
Desired domestic:     1 - flight_fraction
Repatriation:         offshore × max(0, desired_domestic - actual_domestic_share)

Also includes offshore penalty obligations.
*/
v[0] = VL("Household_Deposits_Offshore", 1);
v[16] = 0;  // Default: no repatriation

if(v[0] > 0)
{
    // --- LIQUIDITY COMPONENT ---
    // Obligations: consumption + debt + taxes + penalties (including offshore)
    v[1] = V("Household_Desired_Expenses");
    v[2] = V("Household_Financial_Obligations");
    v[3] = V("Household_Wealth_Tax_Owed");
    v[4] = V("Household_Asset_Penalty");
    v[30] = V("Household_Offshore_Penalty");
    v[5] = VL("Household_Deposits_Domestic", 1);  // Lagged to avoid circular dependency

    v[6] = v[1] + v[2] + v[3] + v[4] + v[30];  // Total obligations
    v[7] = max(0.0, v[6] - v[5]);               // Liquidity shortfall

    // --- DISCRETIONARY COMPONENT (inverse of flight decision) ---
    v[8] = V("household_propensity_evade");                                   // ψ
    v[9] = VS(country, "wealth_tax_rate");                                    // τ_V
    v[10] = VS(centralbank, "Central_Bank_Basic_Interest_Rate");              // r_dom
    v[11] = VS(external, "external_interest_rate");                           // r_off

    // Enforcement deterrence (same as flight decision)
    v[20] = VS(government, "Government_Dynamic_Audit_Probability");           // p
    v[21] = VS(government, "penalty_rate");                                   // ρ
    v[22] = v[20] * v[21];                                                   // p×ρ

    // Total cost per unit
    v[23] = v[22] + max(0.0, v[10] - v[11]);   // p×ρ + Δr

    // Desired flight fraction (same formula as flight decision)
    v[24] = 0;
    if(v[8] > 0 && v[9] > v[23] && v[9] > 0.0001)
        v[24] = v[8] * (1.0 - v[23] / v[9]);   // ψ × (1 - (p×ρ + Δr)/τ_V)
    v[24] = max(0.0, min(1.0, v[24]));

    // Desired domestic share = 1 - flight_fraction
    // If current offshore share exceeds desired, repatriate the excess
    v[25] = v[0] + v[5];  // Total deposits (offshore + domestic)
    v[26] = (v[25] > 0.01) ? v[0] / v[25] : 0;  // Current offshore share
    v[27] = max(0.0, v[26] - v[24]);             // Excess offshore share
    v[14] = v[0] * v[27];                        // Discretionary repatriation

    // Total = max(liquidity, discretionary), capped at offshore stock
    v[15] = max(v[7], v[14]);
    v[16] = min(v[15], v[0]);
}

RESULT(v[16])


EQUATION("Household_Is_Audited")
/*
Stage 9: Stochastic audit outcome. Applies to BOTH asset evasion and offshore deposits.
Uses dynamic audit probability when enforcement_sensitivity > 0.
*/
v[0] = VS(government, "Government_Dynamic_Audit_Probability");
RESULT(RND < v[0] ? 1 : 0)


EQUATION("Household_Asset_Penalty")
/*
Stage 9: Penalty for undeclared ASSETS if caught by audit.

Option A (dissertation): ρ is an absolute confiscation rate on hidden wealth.
Penalty = ρ × Undeclared_Assets

Consistent with evasion formula: h = ψ × (1 - p×ρ/τ_V)
Both use ρ as penalty per unit of hidden wealth, independent of tax rate.
*/
v[0] = V("Household_Is_Audited");
v[1] = V("Household_Assets_Undeclared");
v[4] = 0;  // Default: no penalty

if(v[0] == 1 && v[1] >= 0.01)
{
    v[3] = VS(government, "penalty_rate");
    v[4] = v[3] * v[1];  // ρ × H (absolute confiscation)
}

RESULT(v[4])


EQUATION("Household_Offshore_Penalty")
/*
Stage 9: Penalty for offshore deposits if detected by audit.

Same enforcement structure as asset evasion (Option A):
Penalty = ρ × Offshore_Deposits (absolute confiscation rate).

Under international information exchange (FATCA, CRS), governments can
detect offshore holdings. Same audit draw as domestic assets.
*/
v[0] = V("Household_Is_Audited");
v[1] = VL("Household_Deposits_Offshore", 1);  // Lagged to avoid circular dep
v[4] = 0;  // Default: no penalty

if(v[0] == 1 && v[1] >= 0.01)
{
    v[3] = VS(government, "penalty_rate");
    v[4] = v[3] * v[1];  // ρ × offshore_deposits (absolute confiscation)
}

RESULT(v[4])


/******************************************************************************
 * STAGE 4.7 PREPARATION: EFFECTIVE CONSUMPTION EQUATIONS
 *
 * These equations compute actual consumption (constrained by budget and sector).
 * Added BEFORE the switch to enable validation - should be identical to bridge
 * equations in observer mode.
 *****************************************************************************/


EQUATION("Household_Effective_Expenses")
/*
Stage 4.7: Actual nominal consumption expenses.
Uses EFFECTIVE consumption (after sector rationing), not just demand.

This is the nominal value of what the household actually spends on consumption,
combining domestic and imported goods at their respective prices.
Mirrors Class_Effective_Expenses logic.
*/
v[0] = V("Household_Effective_Real_Domestic_Consumption");   // Sector-rationed domestic
v[1] = V("Household_Effective_Real_Imported_Consumption");   // Sector-rationed imports
v[2] = VLS(consumption, "Sector_Avg_Price", 1);
v[3] = VS(consumption, "Sector_External_Price");
v[4] = VS(external, "Country_Exchange_Rate");

v[5] = v[0]*v[2] + v[1]*v[3]*v[4];  // Nominal expenses
RESULT(max(0, v[5]))


EQUATION("Household_Effective_Real_Domestic_Consumption")
/*
Stage 4.7: What household actually consumed domestically.
May differ from demand if sector was capacity-constrained (rationing).

For now, equals demand. Future enhancement: multiply by Sector_Demand_Met
ratio to account for supply-side rationing when sectors cannot meet demand.
*/
v[0] = V("Household_Real_Domestic_Consumption_Demand");
v[1] = VLS(consumption, "Sector_Demand_Met", 1);  // Rationing ratio (usually 1.0)
RESULT(max(0, v[0] * v[1]))


EQUATION("Household_Effective_Real_Imported_Consumption")
/*
Stage 5.5: Effective imported consumption after rationing.
Mirrors Class_Effective_Real_Imported_Consumption logic.

If domestic demand is not fully met and extra imports are allowed,
household can import additional goods to cover the shortfall.
Otherwise, imported consumption equals desired imported consumption.
*/
v[0] = VS(consumption, "Sector_Demand_Met");              // % of domestic demand met
v[1] = VS(consumption, "Sector_Demand_Met_By_Imports");   // Can extra imports cover shortfall?
v[2] = (1 - v[0]) * v[1];                                 // % of domestic demand met by extra imports
v[3] = V("Household_Real_Domestic_Consumption_Demand");   // Desired domestic consumption
v[4] = V("Household_Real_Imported_Consumption_Demand");   // Desired imported consumption
v[5] = v[2] * v[3] + v[4];                                // Extra imports + desired imports
RESULT(max(0, v[5]))


/******************************************************************************
 * STAGE 5.3: ACTIVE HOUSEHOLD LENDING (HOUSEHOLD_LOANS)
 *
 * Replaces bridge equation with fully functional loan system where households
 * can borrow to smooth consumption. Creates HOUSEHOLD_LOANS objects dynamically.
 *
 * Design principles:
 * - SFC-Correct: All money flows through banks
 * - Pattern consistency: Uses ADDOBJ/CYCLE_SAFE/DELETE like FIRM_LOANS
 * - No household-level credit rationing: Bank-level rationing only
 *
 * Equations organized in two groups:
 * - GROUP D: Financial Obligations (7 equations) - debt service calculations
 * - GROUP E: Loan Demand and Creation (4 equations) - borrowing mechanics
 *****************************************************************************/

// =============================================================================
// GROUP D: FINANCIAL OBLIGATIONS
// =============================================================================

EQUATION("Household_Avg_Debt_Rate")
/*
Stage 5.3: Average debt rate over annual_frequency periods.
Used for risk assessment in interest rate calculation.
*/
v[0] = V("annual_frequency");
v[1] = LAG_AVE(p, "Household_Debt_Rate", v[0], 1);
RESULT(v[1])


EQUATION("Household_Interest_Rate")
/*
Stage 5.3: Household-specific interest rate based on debt risk.
Higher debt rate = higher risk premium = higher interest rate.

Formula: Rate = (1 + debt_rate × risk_premium) × base_rate
This creates procyclical dynamics (Minsky-style):
- Good times: low debt → low rate → more borrowing
- Bad times: high debt → high rate → debt spiral risk
*/
v[0] = VS(financial, "fs_risk_premium_household");
v[1] = V("Household_Avg_Debt_Rate");
v[2] = VS(financial, "Financial_Sector_Avg_Interest_Rate_Short_Term");
v[3] = (1 + v[1] * v[0]) * v[2];
RESULT(max(0, v[3]))


EQUATION("Household_Interest_Payment")
/*
Stage 5.3: Total interest payment on all HOUSEHOLD_LOANS.
Iterates through loan objects and sums interest based on payment type.
*/
v[0] = 0;
v[4] = V("switch_interest_payment");
v[5] = V("Household_Interest_Rate");

if(COUNT("HOUSEHOLD_LOANS") > 0)
{
    CYCLE(cur, "HOUSEHOLD_LOANS")
    {
        v[1] = VS(cur, "household_loan_total_amount");
        if(v[1] > 0)  // Skip dummy loans
        {
            if(v[4] == 0)  // Fixed interest rate (from loan origination)
                v[2] = VS(cur, "household_loan_interest_rate");
            else  // Flexible interest rate (current market rate)
                v[2] = v[5];
            v[3] = v[1] * v[2];
            v[0] += v[3];
        }
    }
}
RESULT(max(0, v[0]))


EQUATION("Household_Debt_Payment")
/*
Stage 5.3: Total amortization on all HOUSEHOLD_LOANS.
Updates loan balances and DELETEs fully paid loans.
Uses CYCLE_SAFE for safe deletion during iteration.
*/
v[0] = 0;

if(COUNT("HOUSEHOLD_LOANS") > 0)
{
    v[7] = COUNT("HOUSEHOLD_LOANS");

    // First pass: Sum all amortizations
    CYCLE(cur, "HOUSEHOLD_LOANS")
    {
        v[8] = VS(cur, "household_loan_total_amount");
        if(v[8] > 0)
            v[0] += VS(cur, "household_loan_fixed_amortization");
    }

    // Second pass: Update balances and delete paid loans
    CYCLE_SAFE(cur, "HOUSEHOLD_LOANS")
    {
        v[4] = VS(cur, "household_loan_total_amount");
        if(v[4] > 0)
        {
            v[5] = VS(cur, "household_loan_fixed_amortization");
            v[6] = v[4] - v[5];

            if(v[6] > 0.01)
                WRITES(cur, "household_loan_total_amount", v[6]);
            else
            {
                // Keep at least one dummy loan object
                if(v[7] > 1)
                    DELETE(cur);
                else
                    WRITES(cur, "household_loan_total_amount", 0);
            }
        }
    }
}
RESULT(max(0, v[0]))


EQUATION("Household_Financial_Obligations")
/*
Stage 5.3: Total financial obligations = Interest + Amortization.
This is deducted from internal funds before consumption.
*/
v[0] = V("Household_Interest_Payment");
v[1] = V("Household_Debt_Payment");
RESULT(v[0] + v[1])


EQUATION("Household_Max_Debt_Rate")
/*
Stage 5.3: Evolving maximum debt rate based on income growth.
Adjusted annually - grows when income grows, shrinks when income falls.
Creates adaptive borrowing capacity tied to economic conditions.
*/
v[0] = V("annual_frequency");
v[1] = fmod((double)t, v[0]);
v[5] = CURRENT;
v[6] = VS(country, "household_debt_rate_adjustment");

if(v[1] == 1)  // Annual adjustment
{
    v[4] = LAG_GROWTH(p, "Household_Nominal_Disposable_Income", v[0], 1);
    if(v[4] > 0)
        v[7] = v[5] + v[6];  // Income growing: expand borrowing capacity
    else if(v[4] < 0)
        v[7] = v[5] - v[6];  // Income falling: contract borrowing capacity
    else
        v[7] = v[5];
}
else
    v[7] = v[5];

RESULT(max(0.05, min(1, v[7])))  // Bound to [0.05, 1.0]


EQUATION("Household_Debt_Rate")
/*
Stage 5.3: Current debt rate = Loans / (Deposits + Avg_Income).
Key risk metric for interest rate calculation.
*/
v[0] = V("Household_Stock_Loans");
v[1] = V("Household_Stock_Deposits");
v[2] = V("Household_Avg_Nominal_Income");
v[3] = v[1] + v[2];

if(v[3] > 0.01)
    v[4] = v[0] / v[3];
else
    v[4] = 1.1;  // Default high risk if no assets/income
RESULT(max(0, v[4]))


// =============================================================================
// GROUP E: LOAN DEMAND AND CREATION
// =============================================================================

EQUATION("Household_Max_Loans")
/*
Stage 5.3: Maximum new loans household can take this period.
Stage 5.4: ADD financial assets to borrowing base (Buy-Borrow-Die).

BBD Strategy: Capitalists can borrow against financial assets instead of
selling them. This allows wealth to grow while maintaining consumption.
*/
v[0] = V("Household_Max_Debt_Rate");
v[1] = VL("Household_Stock_Loans", 1);
v[2] = VL("Household_Stock_Deposits", 1);
v[3] = VL("Household_Nominal_Disposable_Income", 1);

// Stage 5.4 BBD: Add financial assets to borrowing base (capitalists only)
v[10] = V("household_type");
v[11] = 0;
if(v[10] == 1)  // Capitalist
{
    v[11] = VL("Household_Financial_Assets", 1);  // Assets as collateral
}

// Max debt = rate × (deposits + income + assets) - existing loans
v[4] = v[0] * (v[2] + v[3] + v[11]) - v[1];
RESULT(max(0, v[4]))


EQUATION("Household_Demand_Loans")
/*
Stage 5.3: Loan demand = gap between desired expenses and internal funds.
Households borrow when they want to consume more than they can afford.
Bounded by maximum borrowing capacity.
*/
v[0] = V("Household_Desired_Expenses");
v[1] = V("Household_Internal_Funds");
v[2] = V("Household_Max_Loans");

v[3] = v[0] - v[1];  // Gap between desire and means
v[4] = max(0, min(v[3], v[2]));  // Bound by max capacity
RESULT(v[4])


EQUATION("Household_Effective_Loans")
/*
Stage 5.3: Creates HOUSEHOLD_LOANS object for new borrowing.
Loans have fixed amortization over annual_frequency periods (1-year payback).
Interest rate locked at origination (or flexible based on switch).

Stage 7: Also includes WEALTH TAX BORROWING.
Wealth tax borrowing is mandatory (not bounded by max loan capacity).
Creates separate loan for tax payment to maintain tracking.

Total effective loans = Consumption loans + Wealth tax borrowing
*/
v[0] = V("Household_Demand_Loans");        // Consumption-related borrowing
v[1] = V("Household_Interest_Rate");
v[2] = V("annual_frequency");

// Stage 7: Wealth tax borrowing (mandatory, not subject to loan limits)
v[10] = V("Household_Wealth_Tax_From_Borrowing");

// Create consumption loan if needed
if(v[0] > 0.01)
{
    cur = ADDOBJ("HOUSEHOLD_LOANS");
    WRITES(cur, "household_loan_total_amount", v[0]);
    WRITES(cur, "household_loan_interest_rate", v[1]);
    WRITES(cur, "household_loan_fixed_amortization", v[0] / v[2]);  // 1-year payback
}

// Stage 7: Create wealth tax loan if needed (separate loan object)
if(v[10] > 0.01)
{
    cur = ADDOBJ("HOUSEHOLD_LOANS");
    WRITES(cur, "household_loan_total_amount", v[10]);
    WRITES(cur, "household_loan_interest_rate", v[1]);
    WRITES(cur, "household_loan_fixed_amortization", v[10] / v[2]);  // 1-year payback
}

// Total new loans this period
RESULT(v[0] + v[10])


EQUATION("Household_Funds")
/*
Stage 5.3: Total funds available = Internal_Funds + Effective_Loans.
This is what the household can actually spend on consumption.
*/
v[0] = V("Household_Internal_Funds");
v[1] = V("Household_Effective_Loans");
RESULT(v[0] + v[1])


EQUATION("Household_Available_Deposits")
/*
Household available deposits after expenses and financial obligations.
Mirrors Class_Available_Deposits.
*/
v[0] = V("Household_Funds");              // Total funds
v[1] = V("Household_Effective_Expenses"); // Effective expenses
v[2] = max(0, v[0] - v[1]);
RESULT(v[2])


EQUATION("Household_Deposits_Return")
/*
Net return on household deposits.
Interest earned on available + retained deposits.
Mirrors Class_Deposits_Return.
*/
v[0] = V("Household_Available_Deposits");
v[1] = V("Household_Retained_Deposits");
v[2] = VS(financial, "Financial_Sector_Interest_Rate_Deposits");
v[3] = (v[0] + v[1]) * v[2];
RESULT(v[3])


#endif // FUN_HOUSEHOLDS_H
