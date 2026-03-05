/*******************************************************************************
 * fun_diagnostics.h - MMM REBUILD DIAGNOSTIC SYSTEM v2.0
 *
 * Pure household-based logging for secular stagnation analysis.
 * No Class references - all aggregations use HOUSEHOLD objects.
 *
 * VERBOSITY LEVELS:
 *   0 = Dashboard only (key metrics, ~3 lines)
 *   1 = Standard report (~50 lines)
 *   2 = Detailed with sectors (~80 lines)
 *   3 = Debug mode (full dump)
 *
 * EVOLUTION LOG:
 * - v1.0: Original class-based system
 * - v2.0: Pure household-based, secular stagnation focus (2026-01-14)
 ******************************************************************************/

// ============================================================================
// DIAGNOSTIC CONFIGURATION
// ============================================================================
#define DIAG_FREQUENCY      50    // Report every N periods
#define DIAG_VERBOSITY      1     // 0=dashboard, 1=standard, 2=detailed, 3=debug
#define DIAG_SFC_CHECK      1     // Stock-flow consistency checks (1=on)
#define DIAG_EARLY_WARN     1     // Early warning system (1=on)
#define DIAG_TRACK_ENTRY    1     // Track entry/exit dynamics (1=on)

// Thresholds
#define WARN_GDP_IDENTITY   0.05  // 5% GDP identity error = warning
#define WARN_DEBT_RATIO     5.0   // Debt/GDP > 500%
#define WARN_UNEMPLOYMENT   0.25  // Unemployment > 25%
#define WARN_FIRM_EXIT      0.10  // >10% firms exit in period
#define WARN_PRICE_HIGH     10.0  // Price index upper bound
#define WARN_PRICE_LOW      0.1   // Price index lower bound

// Helper macros for NaN/Inf and explosion detection
#define IS_INVALID(x)           (isnan(x) || isinf(x))
#define IS_EXPLOSIVE(x, thr)    (fabs(x) > (thr))
// ============================================================================

// Forward declarations
void log_dashboard(int t);
void log_macro_aggregates();
void log_household_summary();
void log_class_structure();  // Phase 1: CLASS aggregation
void log_income_distribution();
void log_wealth_tax();  // Stage 7
void log_wealth_transfer();  // Stage 7.5
void log_evasion();  // Stage 9
void log_stagnation_metrics();
void log_government_external();
void log_government_budget_detail();
void log_financial_sector();
void log_entry_exit_dynamics(int t);
void log_household_identity_check();
void log_sectoral_detail();
void log_final_summary(int t);
void log_initialization_diagnostics();


/*******************************************************************************
 * log_initialization_diagnostics - Called at end of Initialization_2
 * PRE-FLIGHT CHECKLIST: Verifies model is properly prepared for simulation
 *
 * Focus areas:
 * 1. Identity verification - accounting identities hold at t=0
 * 2. Distribution checks - household parameters properly distributed
 * 3. Balance sheet consistency - financial stocks are coherent
 * 4. Parameter sanity - key parameters are within reasonable ranges
 * 5. Potential issues - flag things that might cause problems
 ******************************************************************************/
void log_initialization_diagnostics()
{
    int errors = 0;
    int warnings = 0;

    LOG("\n");
    LOG("\n╔══════════════════════════════════════════════════════════════════════╗");
    LOG("\n║             INITIALIZATION VERIFICATION - PRE-FLIGHT CHECK           ║");
    LOG("\n╚══════════════════════════════════════════════════════════════════════╝");

    // =========================================================================
    // 1. BASIC STRUCTURE VERIFICATION
    // =========================================================================
    LOG("\n");
    LOG("\n  [1] STRUCTURE VERIFICATION");
    LOG("\n  --------------------------");

    // Check global pointers
    int ptr_ok = 1;
    if (country == NULL) { LOG("\n  [FAIL] country pointer is NULL"); errors++; ptr_ok = 0; }
    if (government == NULL) { LOG("\n  [FAIL] government pointer is NULL"); errors++; ptr_ok = 0; }
    if (financial == NULL) { LOG("\n  [FAIL] financial pointer is NULL"); errors++; ptr_ok = 0; }
    if (external == NULL) { LOG("\n  [FAIL] external pointer is NULL"); errors++; ptr_ok = 0; }
    if (centralbank == NULL) { LOG("\n  [FAIL] centralbank pointer is NULL"); errors++; ptr_ok = 0; }
    if (households == NULL) { LOG("\n  [FAIL] households pointer is NULL"); errors++; ptr_ok = 0; }
    if (consumption == NULL) { LOG("\n  [FAIL] consumption pointer is NULL"); errors++; ptr_ok = 0; }
    if (capital == NULL) { LOG("\n  [FAIL] capital pointer is NULL"); errors++; ptr_ok = 0; }
    if (input == NULL) { LOG("\n  [FAIL] input pointer is NULL"); errors++; ptr_ok = 0; }

    if (ptr_ok) LOG("\n  [OK] All global object pointers initialized");

    // Count agents
    double firm_count = COUNT_ALLS(country, "FIRMS");
    double hh_count = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
    double bank_count = COUNT_ALLS(financial, "BANKS");
    LOG("\n  [INFO] Firms: %.0f  Households: %.0f  Banks: %.0f", firm_count, hh_count, bank_count);

    if (firm_count < 10) { LOG("\n  [WARN] Very few firms (%.0f) - may cause instability", firm_count); warnings++; }
    if (hh_count < 5) { LOG("\n  [WARN] Very few households (%.0f)", hh_count); warnings++; }

    // =========================================================================
    // 2. HOUSEHOLD DISTRIBUTION VERIFICATION
    // =========================================================================
    LOG("\n");
    LOG("\n  [2] HOUSEHOLD DISTRIBUTION CHECKS");
    LOG("\n  ----------------------------------");

    int working_class_count = 0, capitalist_count = 0;
    double sum_profit_shares = 0;
    double sum_skill = 0;
    double total_hh_deposits = 0, total_hh_assets = 0, total_hh_loans = 0;
    double min_deposit = 1e20, max_deposit = -1;
    int zero_deposit_count = 0;

    object *cur, *cur1;
    CYCLES(country, cur1, "CLASSES")  // Use CYCLES with country pointer (CYCLE requires 'p' from equations)
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        double hh_type = VS(cur, "household_type");
        double profit_share = VS(cur, "household_profit_share");
        double skill = VS(cur, "household_skill");
        double deposits = VLS(cur, "Household_Stock_Deposits", 1);
        double assets = VLS(cur, "Household_Financial_Assets", 1);
        double loans = VS(cur, "Household_Stock_Loans");

        if (hh_type == 0) working_class_count++;
        else {
            capitalist_count++;
            sum_profit_shares += profit_share;
        }

        sum_skill += skill;
        total_hh_deposits += deposits;
        total_hh_assets += assets;
        total_hh_loans += loans;

        if (deposits < min_deposit) min_deposit = deposits;
        if (deposits > max_deposit) max_deposit = deposits;
        if (deposits <= 0) zero_deposit_count++;
    }
    }

    int total_hh = working_class_count + capitalist_count;
    double avg_skill = (total_hh > 0) ? sum_skill / total_hh : 0;

    LOG("\n  Workers: %d  Capitalists: %d  Total: %d", working_class_count, capitalist_count, total_hh);

    // Check profit shares sum to 1
    if (capitalist_count > 0) {
        double share_error = fabs(sum_profit_shares - 1.0);
        if (share_error < 0.001)
            LOG("\n  [OK] Profit shares sum to 1.000 (error: %.6f)", share_error);
        else {
            LOG("\n  [FAIL] Profit shares sum to %.4f (should be 1.0)", sum_profit_shares);
            errors++;
        }
    }

    // Check skill distribution
    if (fabs(avg_skill - 1.0) < 0.1)
        LOG("\n  [OK] Avg skill = %.4f (target: 1.0)", avg_skill);
    else {
        LOG("\n  [WARN] Avg skill = %.4f (far from target 1.0)", avg_skill);
        warnings++;
    }

    // Check for zero deposits
    if (zero_deposit_count > 0) {
        LOG("\n  [WARN] %d households have zero or negative deposits", zero_deposit_count);
        warnings++;
    }

    LOG("\n  [INFO] Deposit range: [%.2f, %.2f]", min_deposit, max_deposit);

    // =========================================================================
    // 3. BALANCE SHEET CONSISTENCY
    // =========================================================================
    LOG("\n");
    LOG("\n  [3] BALANCE SHEET CONSISTENCY");
    LOG("\n  ------------------------------");

    // Household deposit sanity check (Class comparison removed Stage 5.5)
    LOG("\n  [INFO] Total HH Deposits: %.2f", total_hh_deposits);
    if (total_hh_deposits < 0) {
        LOG("\n  [WARN] Negative household deposits!");
        warnings++;
    } else {
        LOG("\n  [OK] Household deposits positive");
    }

    // Check financial sector totals
    double fin_deposits = VS(financial, "Financial_Sector_Stock_Deposits");
    double fin_loans = VS(financial, "Financial_Sector_Total_Stock_Loans");

    LOG("\n  [INFO] Financial Sector: Deposits=%.2f  Loans=%.2f  L/D=%.1f%%",
        fin_deposits, fin_loans, (fin_deposits > 0) ? fin_loans/fin_deposits*100 : 0);

    // Check households have no initial loans (by design)
    if (total_hh_loans > 0.01) {
        LOG("\n  [WARN] Households have initial loans (%.2f) - expected 0", total_hh_loans);
        warnings++;
    } else {
        LOG("\n  [OK] Household loans start at zero");
    }

    // =========================================================================
    // 4. GDP IDENTITY CHECK AT t=0
    // =========================================================================
    LOG("\n");
    LOG("\n  [4] GDP IDENTITY AT INITIALIZATION");
    LOG("\n  -----------------------------------");

    double gdp = VS(country, "Country_GDP");
    double wages = VS(country, "Country_Total_Wages");
    double profits = VS(country, "Country_Total_Profits");
    double ind_tax = VS(government, "Government_Indirect_Taxes");
    double gdp_income = wages + profits + ind_tax;

    double income_diff = fabs(gdp - gdp_income);
    double income_pct = (gdp > 0) ? income_diff / gdp * 100 : 0;

    LOG("\n  GDP (parameter):   %.2f", gdp);
    LOG("\n  W + Pi + T:        %.2f  (W=%.2f, Pi=%.2f, T=%.2f)", gdp_income, wages, profits, ind_tax);

    if (income_pct < 1.0)
        LOG("\n  [OK] Income identity holds (%.2f%% error)", income_pct);
    else {
        LOG("\n  [WARN] Income identity error: %.2f%%", income_pct);
        warnings++;
    }

    // =========================================================================
    // 5. KEY PARAMETER SANITY CHECKS
    // =========================================================================
    LOG("\n");
    LOG("\n  [5] PARAMETER SANITY CHECKS");
    LOG("\n  ---------------------------");

    double switch_het = VS(country, "switch_household_heterogeneity");
    double price_idx = VS(country, "Country_Price_Index");
    double interest_rate = VS(centralbank, "Central_Bank_Basic_Interest_Rate");
    double govt_debt = VS(government, "Government_Debt");

    LOG("\n  Mode: %s", switch_het == 1 ? "HETEROGENEOUS" : "HOMOGENEOUS");

    // Price index should be ~1.0 at start
    if (fabs(price_idx - 1.0) < 0.5)
        LOG("\n  [OK] Price index = %.4f (near 1.0)", price_idx);
    else {
        LOG("\n  [WARN] Price index = %.4f (far from 1.0)", price_idx);
        warnings++;
    }

    // Interest rate should be reasonable
    if (interest_rate >= 0 && interest_rate < 0.1)
        LOG("\n  [OK] Interest rate = %.4f (%.2f%%)", interest_rate, interest_rate * 100);
    else {
        LOG("\n  [WARN] Interest rate = %.4f seems unusual", interest_rate);
        warnings++;
    }

    // Debt/GDP ratio
    double debt_ratio = (gdp > 0) ? govt_debt / gdp : 0;
    if (debt_ratio < 2.0)
        LOG("\n  [OK] Debt/GDP = %.1f%%", debt_ratio * 100);
    else {
        LOG("\n  [WARN] Debt/GDP = %.1f%% (high initial debt)", debt_ratio * 100);
        warnings++;
    }

    // =========================================================================
    // 6. SECTOR CONFIGURATION
    // =========================================================================
    LOG("\n");
    LOG("\n  [6] SECTOR CONFIGURATION");
    LOG("\n  ------------------------");

    CYCLES(country, cur, "SECTORS")
    {
        const char* sec_name;
        if (VS(cur, "id_consumption_goods_sector") == 1) sec_name = "Consumption";
        else if (VS(cur, "id_capital_goods_sector") == 1) sec_name = "Capital";
        else sec_name = "Intermediate";

        double firms = COUNTS(cur, "FIRMS");
        double profit_rate = VS(cur, "sector_initial_profit_rate");
        double des_util = VS(cur, "sector_desired_degree_capacity_utilization");

        LOG("\n  %-12s: %3.0f firms, profit_rate=%.2f, target_util=%.0f%%",
            sec_name, firms, profit_rate, des_util * 100);

        if (firms < 5) {
            LOG("\n    [WARN] Very few firms in %s sector", sec_name);
            warnings++;
        }
    }

    // =========================================================================
    // 7. POTENTIAL SIMULATION ISSUES
    // =========================================================================
    LOG("\n");
    LOG("\n  [7] POTENTIAL ISSUES DETECTED");
    LOG("\n  -----------------------------");

    // Check for growth drivers that might prevent stagnation
    double govt_growth = VS(government, "government_real_investment_growth");
    double ext_growth = VS(external, "external_income_growth");

    if (govt_growth > 0.001) {
        LOG("\n  [NOTE] Govt investment growth = %.3f%% - may prevent stagnation", govt_growth * 100);
    }
    if (ext_growth > 0.001) {
        LOG("\n  [NOTE] External income growth = %.3f%% - may prevent stagnation", ext_growth * 100);
    }

    // Check wealth concentration setup
    double cap_wealth_share = VS(country, "Country_Capitalist_Wealth_Share");
    LOG("\n  [INFO] Initial capitalist wealth share: %.1f%%", cap_wealth_share * 100);

    // =========================================================================
    // SUMMARY
    // =========================================================================
    LOG("\n");
    LOG("\n  ════════════════════════════════════════════");
    if (errors == 0 && warnings == 0) {
        LOG("\n  ✓ ALL CHECKS PASSED - Model ready for simulation");
    } else if (errors == 0) {
        LOG("\n  ⚠ PASSED WITH %d WARNING(S) - Review above", warnings);
    } else {
        LOG("\n  ✗ %d ERROR(S), %d WARNING(S) - FIX BEFORE RUNNING", errors, warnings);
    }
    LOG("\n  ════════════════════════════════════════════");
    LOG("\n");
}


/*******************************************************************************
 * log_diagnostic_report - Main entry point
 * Called from Diagnostic_Trigger equation (after Country_GDP computed)
 ******************************************************************************/
void log_diagnostic_report(object *p, int t)
{
    if (DIAG_VERBOSITY < 0) return;

    int is_periodic = (t > 1 && (t % DIAG_FREQUENCY) == 0);
    int is_final = (t == LAST_T);

    if (!is_periodic && !is_final) return;

    // =========================================================================
    // DASHBOARD (Always shown)
    // =========================================================================
    log_dashboard(t);

    // =========================================================================
    // STANDARD REPORT (Verbosity >= 1)
    // =========================================================================
    if (DIAG_VERBOSITY >= 1)
    {
        log_macro_aggregates();
        log_household_summary();
        log_class_structure();  // Phase 1: CLASS aggregation
        log_income_distribution();
        log_wealth_tax();  // Stage 7
        log_wealth_transfer();  // Stage 7.5
        log_evasion();  // Stage 9
        log_stagnation_metrics();
        log_government_external();
        log_government_budget_detail();
        log_financial_sector();

        if (DIAG_TRACK_ENTRY)
            log_entry_exit_dynamics(t);

        if (DIAG_SFC_CHECK)
        {
            log_household_identity_check();
        }
    }

    // =========================================================================
    // DETAILED REPORT (Verbosity >= 2)
    // =========================================================================
    if (DIAG_VERBOSITY >= 2)
    {
        log_sectoral_detail();
    }

    // =========================================================================
    // FINAL SUMMARY
    // =========================================================================
    if (is_final)
    {
        log_final_summary(t);
    }

    LOG("\n+===============================================+\n");
}


/*******************************************************************************
 * log_dashboard - Single-line key metrics (always shown)
 * Note: Diagnostics now run from Diagnostic_Trigger which depends on Country_GDP,
 *       so most variables can be accessed with VS() (current value).
 *       Country_GDP uses VL() for growth calculations (needs lag).
 ******************************************************************************/
void log_dashboard(int t)
{
    double gdp = VS(country, "Country_GDP");  // Already computed via Diagnostic_Trigger
    double gdp_prev = VLS(country, "Country_GDP", 1);   // Lagged for growth calculation
    double gdp_growth = (gdp_prev > 0) ? ((gdp - gdp_prev) / gdp_prev) * 100 : 0;

    double firm_count = COUNT_ALLS(country, "FIRMS");
    double u_rate = VS(country, "Country_Unemployment_Rate");

    double v = VS(country, "Country_Wealth_Capital_Ratio");
    double beta = VS(country, "Country_Wealth_GDP_Ratio");
    double cap_share = VS(country, "Country_Capitalist_Wealth_Share");
    double gini = VS(country, "Country_Gini_Index");

    double gdp_income = VS(country, "Country_GDP");
    double gdp_demand = VS(country, "Country_GDP_Demand");
    double sfc_pct = (gdp_income > 0) ? fabs(gdp_demand - gdp_income) / gdp_income * 100 : 0;

    char growth_arrow = (gdp_growth >= 0) ? '+' : ' ';
    char sfc_status = (sfc_pct < WARN_GDP_IDENTITY * 100) ? 'O' : 'X';

    LOG("\n");
    LOG("\n+==================== t=%-4d ====================+", t);
    LOG("\n| GDP: %8.0f (%c%.1f%%) | Firms: %3.0f | U: %4.1f%% |",
        gdp, growth_arrow, gdp_growth, firm_count, u_rate * 100);
    LOG("\n| v=%4.2f | B=%4.2f | CapSh: %4.1f%% | Gini: %.2f |",
        v, beta, cap_share * 100, gini);
    LOG("\n| SFC: %c (%.1f%%) | C+I+G+NX vs W+Pi+T             |",
        sfc_status, sfc_pct);
    LOG("\n+===============================================+");
}


/*******************************************************************************
 * log_macro_aggregates - Core macroeconomic variables
 ******************************************************************************/
void log_macro_aggregates()
{
    double gdp = VS(country, "Country_GDP");
    double gdp_real = VS(country, "Country_Real_GDP");
    double price_idx = VS(country, "Country_Price_Index");
    double inflation = VS(country, "Country_Annual_Inflation");
    double cap_util = VS(country, "Country_Capacity_Utilization");
    double productivity = VS(country, "Country_Avg_Productivity");
    double capital = VS(country, "Country_Capital_Stock");

    LOG("\n");
    LOG("\n  MACRO AGGREGATES");
    LOG("\n  ----------------");
    LOG("\n  GDP (Nominal):    %12.2f", gdp);
    LOG("\n  GDP (Real):       %12.2f", gdp_real);
    LOG("\n  Price Index:      %12.4f", price_idx);
    LOG("\n  Inflation (Ann):  %12.2f%%", inflation * 100);
    LOG("\n  Capacity Util:    %12.1f%%", cap_util * 100);
    LOG("\n  Avg Productivity: %12.4f", productivity);
    LOG("\n  Capital Stock:    %12.2f", capital);

    // Early warnings
    if (DIAG_EARLY_WARN)
    {
        if (price_idx > WARN_PRICE_HIGH)
            LOG("\n  >>> WARNING: Price Index approaching hyperinflation <<<");
        if (price_idx < WARN_PRICE_LOW)
            LOG("\n  >>> WARNING: Price Index approaching deflation <<<");
    }
}


/*******************************************************************************
 * log_household_summary - Pure household-based (no class references)
 ******************************************************************************/
void log_household_summary()
{
    if (households == NULL) return;

    int working_class_count = 0;
    int capitalist_count = 0;
    int employed_count = 0;
    int unemployed_count = 0;
    double total_income = 0;
    double total_wealth = 0;

    object *cur, *cur1;
    CYCLES(country, cur1, "CLASSES")  // Use CYCLES with country pointer (CYCLE requires 'p' from equations)
    {
    CYCLES(cur1, cur, "HOUSEHOLD")
    {
        double hh_type = VS(cur, "household_type");  // Parameter
        double status = VS(cur, "Household_Employment_Status");
        double income = VS(cur, "Household_Nominal_Disposable_Income");
        double deposits = VS(cur, "Household_Stock_Deposits");
        double assets = VS(cur, "Household_Financial_Assets");
        double loans = VS(cur, "Household_Stock_Loans");

        if (hh_type == 0)  // Worker
        {
            working_class_count++;
            if (status == 1) employed_count++;
            else if (status == 0) unemployed_count++;
        }
        else  // Capitalist
        {
            capitalist_count++;
        }

        total_income += income;
        total_wealth += deposits + assets - loans;
    }
    }

    int total_hh = working_class_count + capitalist_count;
    double emp_rate = (working_class_count > 0) ? (double)employed_count / working_class_count * 100 : 0;
    double u_rate = (working_class_count > 0) ? (double)unemployed_count / working_class_count * 100 : 0;

    LOG("\n");
    LOG("\n  HOUSEHOLDS");
    LOG("\n  ----------");
    LOG("\n  Total:            %12d", total_hh);
    LOG("\n  Workers:          %12d", working_class_count);
    LOG("\n  Capitalists:      %12d", capitalist_count);
    LOG("\n  Employed:         %8d (%5.1f%%)", employed_count, emp_rate);
    LOG("\n  Unemployed:       %8d (%5.1f%%)", unemployed_count, u_rate);
    LOG("\n  Total Income:     %12.2f", total_income);
    LOG("\n  Total Net Wealth: %12.2f", total_wealth);

    if (DIAG_EARLY_WARN && u_rate / 100.0 > WARN_UNEMPLOYMENT)
        LOG("\n  >>> WARNING: High unemployment rate! <<<");
}


/*******************************************************************************
 * log_class_structure - Phase 1: CLASS-level aggregation diagnostics
 * Uses the new CLASS structure (CLASSES → CLASS → HOUSEHOLD)
 ******************************************************************************/
void log_class_structure()
{
    // Only report if CLASS structure is initialized
    if (working_class == NULL || capitalist_class == NULL)
    {
        LOG("\n");
        LOG("\n  CLASS STRUCTURE: Not initialized (LSD setup required)");
        return;
    }

    // Get class counts
    double w_count = COUNTS(working_class, "HOUSEHOLD");
    double c_count = COUNTS(capitalist_class, "HOUSEHOLD");
    double total = w_count + c_count;

    // Get class-level aggregates
    double w_income = VS(working_class, "Class_Nominal_Disposable_Income");
    double c_income = VS(capitalist_class, "Class_Nominal_Disposable_Income");
    double total_income = w_income + c_income;

    double w_wealth = VS(working_class, "Class_Net_Wealth");
    double c_wealth = VS(capitalist_class, "Class_Net_Wealth");
    double total_wealth = w_wealth + c_wealth;

    double w_consumption = VS(working_class, "Class_Effective_Expenses");
    double c_consumption = VS(capitalist_class, "Class_Effective_Expenses");
    double total_consumption = w_consumption + c_consumption;

    // Employment (working_class only)
    double w_employed = VS(working_class, "Class_Employed_Count");
    double w_unemployed = VS(working_class, "Class_Unemployed_Count");
    double u_rate = (w_count > 0) ? w_unemployed / w_count * 100 : 0;

    // Averages
    double w_avg_income = VS(working_class, "Class_Avg_Disposable_Income");
    double c_avg_income = VS(capitalist_class, "Class_Avg_Disposable_Income");
    double w_avg_wealth = VS(working_class, "Class_Avg_Net_Wealth");
    double c_avg_wealth = VS(capitalist_class, "Class_Avg_Net_Wealth");

    LOG("\n");
    LOG("\n  CLASS STRUCTURE (Phase 1)");
    LOG("\n  -------------------------");
    LOG("\n                        Workers      Capitalists       Total");
    LOG("\n  Count:           %12.0f     %12.0f  %12.0f", w_count, c_count, total);
    LOG("\n  Share:           %11.1f%%    %11.1f%%", w_count/total*100, c_count/total*100);
    LOG("\n");
    LOG("\n  Income:          %12.2f     %12.2f  %12.2f", w_income, c_income, total_income);
    LOG("\n  Income Share:    %11.1f%%    %11.1f%%",
        (total_income > 0) ? w_income/total_income*100 : 0,
        (total_income > 0) ? c_income/total_income*100 : 0);
    LOG("\n  Avg Income:      %12.2f     %12.2f", w_avg_income, c_avg_income);
    LOG("\n");
    LOG("\n  Wealth:          %12.2f     %12.2f  %12.2f", w_wealth, c_wealth, total_wealth);
    LOG("\n  Wealth Share:    %11.1f%%    %11.1f%%",
        (total_wealth > 0) ? w_wealth/total_wealth*100 : 0,
        (total_wealth > 0) ? c_wealth/total_wealth*100 : 0);
    LOG("\n  Avg Wealth:      %12.2f     %12.2f", w_avg_wealth, c_avg_wealth);
    LOG("\n");
    LOG("\n  Consumption:     %12.2f     %12.2f  %12.2f", w_consumption, c_consumption, total_consumption);
    LOG("\n  Employed:        %12.0f     %12s", w_employed, "-");
    LOG("\n  Unemployed:      %12.0f (%5.1f%%)", w_unemployed, u_rate);
}


/*******************************************************************************
 * log_income_distribution - Wages, profits, functional shares
 ******************************************************************************/
void log_income_distribution()
{
    double wages = VS(country, "Country_Total_Wages");
    double profits = VS(country, "Country_Total_Profits");
    double wage_share = VS(country, "Country_Wage_Share");
    double profit_share = VS(country, "Country_Profit_Share");
    double gini_income = VS(country, "Country_Gini_Index");
    double gini_pretax = VS(country, "Country_Gini_Index_Pretax");
    double gini_wealth = VS(country, "Country_Gini_Index_Wealth");

    LOG("\n");
    LOG("\n  INCOME DISTRIBUTION");
    LOG("\n  -------------------");
    LOG("\n  Total Wages:      %12.2f", wages);
    LOG("\n  Total Profits:    %12.2f", profits);
    LOG("\n  Wage Share:       %12.1f%%", wage_share * 100);
    LOG("\n  Profit Share:     %12.1f%%", profit_share * 100);
    LOG("\n  Gini (Post-Tax):  %12.3f", gini_income);
    LOG("\n  Gini (Pre-Tax):   %12.3f", gini_pretax);
    LOG("\n  Gini (Wealth):    %12.3f", gini_wealth);

    // Plausibility range checks (catches NaN, zero, explosion, master not running)
    if(IS_INVALID(gini_income) || gini_income < 0.1 || gini_income > 0.99)
        LOG("\n  *** WARNING: Country_Gini_Index out of plausible range: %.4f", gini_income);
    if(IS_INVALID(gini_pretax) || gini_pretax < 0.1 || gini_pretax > 0.99)
        LOG("\n  *** WARNING: Country_Gini_Index_Pretax out of plausible range: %.4f", gini_pretax);
    if(IS_INVALID(gini_wealth) || gini_wealth < 0.1 || gini_wealth > 0.99)
        LOG("\n  *** WARNING: Country_Gini_Index_Wealth out of plausible range: %.4f", gini_wealth);
}


/*******************************************************************************
 * log_wealth_tax - Stage 7: Wealth tax metrics
 ******************************************************************************/
void log_wealth_tax()
{
    double tax_switch = VS(country, "switch_class_tax_structure");
    if (tax_switch < 5)
    {
        LOG("\n");
        LOG("\n  WEALTH TAX [INACTIVE]");
        LOG("\n  ---------------------");
        LOG("\n  (switch_class_tax_structure < 5)");
        return;
    }

    double revenue = VS(country, "Country_Wealth_Tax_Revenue");
    double from_deposits = VS(country, "Country_Wealth_Tax_From_Deposits");
    double from_assets = VS(country, "Country_Wealth_Tax_From_Assets");
    double from_borrowing = VS(country, "Country_Wealth_Tax_From_Borrowing");
    double taxpayer_count = VS(country, "Country_Wealth_Tax_Taxpayer_Count");
    double threshold = VS(country, "wealth_tax_threshold");
    double rate = VS(country, "wealth_tax_rate");
    double govt_revenue = VS(government, "Government_Wealth_Tax_Revenue");

    // Calculate percentages of payment sources
    double total_sources = from_deposits + from_assets + from_borrowing;
    double pct_deposits = (total_sources > 0) ? from_deposits / total_sources * 100 : 0;
    double pct_assets = (total_sources > 0) ? from_assets / total_sources * 100 : 0;
    double pct_borrowing = (total_sources > 0) ? from_borrowing / total_sources * 100 : 0;

    LOG("\n");
    LOG("\n  WEALTH TAX [ACTIVE]");
    LOG("\n  -------------------");
    LOG("\n  Tax Rate:         %12.2f%%", rate * 100);
    LOG("\n  Threshold:        %12.2f", threshold);
    LOG("\n  Taxpayers:        %12.0f", taxpayer_count);
    LOG("\n  Total Revenue:    %12.2f", revenue);
    LOG("\n  Govt Revenue:     %12.2f", govt_revenue);
    LOG("\n  --- PAYMENT SOURCES ---");
    LOG("\n  From Deposits:    %12.2f (%5.1f%%)", from_deposits, pct_deposits);
    LOG("\n  From Assets:      %12.2f (%5.1f%%)", from_assets, pct_assets);
    LOG("\n  From Borrowing:   %12.2f (%5.1f%%)", from_borrowing, pct_borrowing);

    // Consistency check
    if (fabs(revenue - govt_revenue) > 0.01)
        LOG("\n  >>> WARNING: Revenue mismatch! <<<");
}


/*******************************************************************************
 * log_wealth_transfer - Stage 7.5: Equal distribution to bottom X%
 ******************************************************************************/
void log_wealth_transfer()
{
    double tax_switch = VS(country, "switch_class_tax_structure");
    if (tax_switch < 5)
    {
        LOG("\n");
        LOG("\n  WEALTH TRANSFER [INACTIVE]");
        LOG("\n  --------------------------");
        LOG("\n  (switch_class_tax_structure < 5)");
        return;
    }

    // Get target percentile parameter
    double target_pct = VS(country, "wealth_transfer_target_percentile");
    if (target_pct <= 0 || target_pct > 1) target_pct = 0.5;

    double desired = VS(country, "Country_Transfer_Desired");
    double eligible = VS(country, "Country_Transfer_Eligible");
    double effective = VS(government, "Government_Effective_Wealth_Transfer");

    // Calculate scale factor (budget constraint)
    double scale = (desired > 0.01) ? effective / desired : 1.0;

    // Calculate total households for context
    double total_hh = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
    double pct_eligible = (total_hh > 0) ? eligible / total_hh * 100 : 0;

    // Equal transfer per eligible household
    double transfer_each = (eligible > 0) ? effective / eligible : 0;

    // Calculate income threshold (same as Country_Transfer_Desired)
    double threshold = household_percentile(country, "Household_Avg_Real_Income", target_pct, 1);

    LOG("\n");
    LOG("\n  WEALTH TRANSFER [EQUAL TO BOTTOM %.0f%%]", target_pct * 100);
    LOG("\n  -------------------------------------");
    LOG("\n  Target Percentile: %11.0f%%", target_pct * 100);
    LOG("\n  Income Threshold:  %11.2f", threshold);
    LOG("\n  Eligible HH:       %11.0f (%5.1f%% of total)", eligible, pct_eligible);
    LOG("\n  Desired (revenue): %11.2f", desired);
    LOG("\n  Effective Budget:  %11.2f", effective);
    LOG("\n  Scale Factor:      %11.2f%%", scale * 100);
    LOG("\n  Transfer Each:     %11.2f", transfer_each);

    // Warning if significant budget constraint
    if (scale < 0.9)
        LOG("\n  >>> NOTE: Transfer reduced by budget constraint <<<");
}


/*******************************************************************************
 * log_evasion - Stage 9: Tax evasion and capital flight metrics
 ******************************************************************************/
void log_evasion()
{
    double tax_switch = VS(country, "switch_class_tax_structure");
    if (tax_switch < 5)
    {
        LOG("\n");
        LOG("\n  TAX EVASION [INACTIVE]");
        LOG("\n  ----------------------");
        LOG("\n  (switch_class_tax_structure < 5)");
        return;
    }

    // Capital Flight (Deposits)
    double offshore = VS(country, "Country_Total_Deposits_Offshore");
    double total_deposits = VS(country, "Country_Total_Household_Stock_Deposits");
    double flight_rate = VS(country, "Country_Capital_Flight_Rate");

    // Asset Evasion (Financial Assets)
    double undeclared = VS(country, "Country_Total_Assets_Undeclared");
    double declared = VS(country, "Country_Total_Assets_Declared");
    double total_assets = VS(country, "Country_Total_Financial_Assets");
    double evasion_rate = VS(country, "Country_Asset_Evasion_Rate");

    // Enforcement
    double evader_count = VS(country, "Country_Evader_Count");
    double audit_count = VS(country, "Country_Audit_Count");
    double penalty_revenue = VS(country, "Country_Penalty_Revenue");
    double tax_gap = VS(government, "Government_Wealth_Tax_Gap");

    // Parameters
    double audit_prob_base = VS(government, "audit_probability");
    double audit_prob_dynamic = VS(government, "Government_Dynamic_Audit_Probability");
    double enforcement_sens = VS(government, "enforcement_sensitivity");
    double penalty_rate = VS(government, "penalty_rate");
    double offshore_rate = VS(external, "external_interest_rate");
    double domestic_rate = VS(centralbank, "Central_Bank_Basic_Interest_Rate");

    // Total households for context
    double total_hh = COUNTS(working_class, "HOUSEHOLD") + COUNTS(capitalist_class, "HOUSEHOLD");
    double pct_evaders = (total_hh > 0) ? evader_count / total_hh * 100 : 0;

    LOG("\n");
    LOG("\n  TAX EVASION (Stage 9)");
    LOG("\n  ---------------------");
    LOG("\n  --- CAPITAL FLIGHT ---");
    LOG("\n  Offshore Deposits: %12.2f", offshore);
    LOG("\n  Total Deposits:    %12.2f", total_deposits);
    LOG("\n  Flight Rate:       %12.2f%%", flight_rate * 100);
    LOG("\n  Interest Spread:   %12.2f%% (dom=%.2f%%, off=%.2f%%)",
        (domestic_rate - offshore_rate) * 100, domestic_rate * 100, offshore_rate * 100);
    LOG("\n  --- ASSET NON-DECLARATION ---");
    LOG("\n  Declared Assets:   %12.2f", declared);
    LOG("\n  Undeclared Assets: %12.2f", undeclared);
    LOG("\n  Total Assets:      %12.2f", total_assets);
    LOG("\n  Evasion Rate:      %12.2f%%", evasion_rate * 100);
    LOG("\n  Deterrence (p×π):  %12.2f", audit_prob_dynamic * penalty_rate);
    LOG("\n  --- ENFORCEMENT ---");
    LOG("\n  Audit Prob (base): %12.2f%%", audit_prob_base * 100);
    LOG("\n  Audit Prob (dyn):  %12.2f%% (sens=%.1f)", audit_prob_dynamic * 100, enforcement_sens);
    LOG("\n  Evaders:           %12.0f (%5.1f%% of HH)", evader_count, pct_evaders);
    LOG("\n  Audited:           %12.0f", audit_count);
    LOG("\n  Penalty Revenue:   %12.2f", penalty_revenue);
    LOG("\n  Tax Gap:           %12.2f", tax_gap);

    // Warnings
    if (flight_rate > 0.3)
        LOG("\n  >>> WARNING: High capital flight (>30%%) <<<");
    if (evasion_rate > 0.3)
        LOG("\n  >>> WARNING: High asset evasion (>30%%) <<<");
}


/*******************************************************************************
 * log_stagnation_metrics - KEY: Secular stagnation indicators
 ******************************************************************************/
void log_stagnation_metrics()
{
    double v = VS(country, "Country_Wealth_Capital_Ratio");
    double beta = VS(country, "Country_Wealth_GDP_Ratio");
    double cap_share = VS(country, "Country_Capitalist_Wealth_Share");
    double asset_infl = VS(financial, "Financial_Asset_Inflation");
    double valuation = VS(financial, "Financial_Sector_Valuation_Ratio");

    // Interpret financialization level
    const char* v_status;
    if (v > 3.0) v_status = "[HIGHLY FINANCIALIZED]";
    else if (v > 2.0) v_status = "[FINANCIALIZED]";
    else if (v > 1.5) v_status = "[ELEVATED]";
    else v_status = "[NORMAL]";

    LOG("\n");
    LOG("\n  SECULAR STAGNATION INDICATORS");
    LOG("\n  ------------------------------");
    LOG("\n  Valuation (v):    %8.2f  %s", v, v_status);
    LOG("\n  Wealth/GDP (B):   %8.2f", beta);
    LOG("\n  Cap Wealth Share: %8.1f%%", cap_share * 100);
    LOG("\n  Asset Inflation:  %8.2f%%", asset_infl * 100);
    LOG("\n  Fin Valuation:    %8.2f", valuation);
}


/*******************************************************************************
 * log_government_external - Government and external sector
 ******************************************************************************/
void log_government_external()
{
    double govt_debt = VS(government, "Government_Debt");
    double govt_taxes = VS(government, "Government_Total_Taxes");
    double govt_exp = VS(government, "Government_Effective_Expenses");
    double interest_rate = VS(centralbank, "Central_Bank_Basic_Interest_Rate");
    double gdp = VS(country, "Country_GDP");
    double debt_ratio = (gdp > 0) ? govt_debt / gdp : 0;

    double exports = VS(country, "Country_Nominal_Exports");
    double imports = VS(country, "Country_Nominal_Imports");
    double trade_bal = exports - imports;

    LOG("\n");
    LOG("\n  GOVERNMENT                    EXTERNAL SECTOR");
    LOG("\n  ----------                    ---------------");
    LOG("\n  Debt:       %10.2f        Exports:     %10.2f", govt_debt, exports);
    LOG("\n  Taxes:      %10.2f        Imports:     %10.2f", govt_taxes, imports);
    LOG("\n  Expenses:   %10.2f        Trade Bal:   %10.2f", govt_exp, trade_bal);
    LOG("\n  Debt/GDP:   %10.1f%%       Int Rate:    %10.2f%%", debt_ratio * 100, interest_rate * 100);

    if (DIAG_EARLY_WARN && debt_ratio > WARN_DEBT_RATIO)
        LOG("\n  >>> WARNING: High government debt ratio! <<<");
}


/*******************************************************************************
 * log_government_budget_detail - Budget constraint monitoring
 * Tracks desired vs effective spending, constraint status, and edge cases
 ******************************************************************************/
void log_government_budget_detail()
{
    // Get desired spending categories
    double des_wages = VS(government, "Government_Desired_Wages");
    double des_benefits = VS(government, "Government_Desired_Unemployment_Benefits");
    double des_consumption = VS(government, "Government_Desired_Consumption");
    double des_inputs = VS(government, "Government_Desired_Inputs");
    double des_investment = VS(government, "Government_Desired_Investment");
    double total_desired = des_wages + des_benefits + des_consumption + des_inputs + des_investment;

    // Get effective spending categories
    double eff_wages = VS(government, "Government_Effective_Wages");
    double eff_benefits = VS(government, "Government_Effective_Unemployment_Benefits");
    double eff_consumption = VS(government, "Government_Effective_Consumption");
    double eff_inputs = VS(government, "Government_Effective_Inputs");
    double eff_investment = VS(government, "Government_Effective_Investment");
    double total_effective = eff_wages + eff_benefits + eff_consumption + eff_inputs + eff_investment;

    // Get budget limit
    double max_expenses = VS(government, "Government_Max_Expenses");

    // Determine constraint status
    // max_expenses = -1 means no fiscal rule active
    int is_constrained = (max_expenses > 0 && total_desired > max_expenses);

    double shortfall = total_desired - max_expenses;
    double shortfall_pct = (total_desired > 0) ? shortfall / total_desired * 100 : 0;

    // Check for edge cases (categories clipped to zero)
    int wages_clipped = (des_wages > 0 && eff_wages <= 0);
    int benefits_clipped = (des_benefits > 0 && eff_benefits <= 0);
    int consumption_clipped = (des_consumption > 0 && eff_consumption <= 0);
    int inputs_clipped = (des_inputs > 0 && eff_inputs <= 0);
    int investment_clipped = (des_investment > 0 && eff_investment <= 0);
    int any_clipped = wages_clipped || benefits_clipped || consumption_clipped || inputs_clipped || investment_clipped;

    // Calculate cut percentages
    double wage_cut = (des_wages > 0) ? (1 - eff_wages / des_wages) * 100 : 0;
    double benefit_cut = (des_benefits > 0) ? (1 - eff_benefits / des_benefits) * 100 : 0;
    double consumption_cut = (des_consumption > 0) ? (1 - eff_consumption / des_consumption) * 100 : 0;
    double inputs_cut = (des_inputs > 0) ? (1 - eff_inputs / des_inputs) * 100 : 0;
    double investment_cut = (des_investment > 0) ? (1 - eff_investment / des_investment) * 100 : 0;

    LOG("\n");
    LOG("\n  GOVERNMENT BUDGET DETAIL");
    LOG("\n  ------------------------");
    LOG("\n  Status: %s", is_constrained ? "CONSTRAINED" : "UNCONSTRAINED");
    if (is_constrained) {
        LOG("\n  Max Budget: %10.2f  Desired: %10.2f  Gap: %.1f%%", max_expenses, total_desired, shortfall_pct);
    }

    LOG("\n  %-12s %10s %10s %8s", "Category", "Desired", "Effective", "Cut %");
    LOG("\n  %-12s %10.2f %10.2f %7.1f%%", "Wages", des_wages, eff_wages, wage_cut);
    LOG("\n  %-12s %10.2f %10.2f %7.1f%%", "Benefits", des_benefits, eff_benefits, benefit_cut);
    LOG("\n  %-12s %10.2f %10.2f %7.1f%%", "Consumption", des_consumption, eff_consumption, consumption_cut);
    LOG("\n  %-12s %10.2f %10.2f %7.1f%%", "Inputs", des_inputs, eff_inputs, inputs_cut);
    LOG("\n  %-12s %10.2f %10.2f %7.1f%%", "Investment", des_investment, eff_investment, investment_cut);
    LOG("\n  %-12s %10.2f %10.2f", "TOTAL", total_desired, total_effective);

    // Edge case warnings
    if (any_clipped) {
        LOG("\n  >>> EDGE CASE: Categories clipped to zero:");
        if (wages_clipped) LOG(" Wages");
        if (benefits_clipped) LOG(" Benefits");
        if (consumption_clipped) LOG(" Consumption");
        if (inputs_clipped) LOG(" Inputs");
        if (investment_clipped) LOG(" Investment");
        LOG(" <<<");
    }

    // Check if total effective doesn't match max (clipping caused underspend)
    if (is_constrained && total_effective < max_expenses * 0.99) {
        double underspend = max_expenses - total_effective;
        LOG("\n  >>> NOTE: Budget underspent by %.2f due to clipping <<<", underspend);
    }
}


/*******************************************************************************
 * log_financial_sector - Banking and financial metrics
 ******************************************************************************/
void log_financial_sector()
{
    double deposits = VS(financial, "Financial_Sector_Stock_Deposits");
    double loans = VS(financial, "Financial_Sector_Total_Stock_Loans");
    double default_rate = VS(financial, "Financial_Sector_Default_Rate");
    double leverage = VS(financial, "Financial_Sector_Leverage");
    double demand_met = VS(financial, "Financial_Sector_Demand_Met");

    double ld_ratio = (deposits > 0) ? loans / deposits * 100 : 0;

    LOG("\n");
    LOG("\n  FINANCIAL SECTOR");
    LOG("\n  ----------------");
    LOG("\n  Total Deposits:   %12.2f", deposits);
    LOG("\n  Total Loans:      %12.2f", loans);
    LOG("\n  Loan/Deposit:     %12.1f%%", ld_ratio);
    LOG("\n  Default Rate:     %12.2f%%", default_rate * 100);
    LOG("\n  Leverage:         %12.2f", leverage);
    LOG("\n  Demand Met:       %12.1f%%", demand_met * 100);
}


/*******************************************************************************
 * log_entry_exit_dynamics - Firm entry/exit tracking
 ******************************************************************************/
void log_entry_exit_dynamics(int t)
{
    double exits = VS(country, "Exit");
    double bankruptcies = VS(country, "Exit_Bankruptcy_Events");
    double firm_count = COUNT_ALLS(country, "FIRMS");

    // Sum entries across sectors
    double entries = 0;
    object *cur;
    CYCLES(country, cur, "SECTORS")
    {
        entries += VS(cur, "Sector_Productive_Capacity_Entry");
    }

    double exit_rate = (firm_count > 0) ? exits / firm_count * 100 : 0;

    LOG("\n");
    LOG("\n  FIRM DYNAMICS");
    LOG("\n  -------------");
    LOG("\n  Total Firms:      %12.0f", firm_count);
    LOG("\n  Exits (period):   %12.0f (%.1f%%)", exits, exit_rate);
    LOG("\n  Entries (period): %12.0f", entries);
    LOG("\n  Bankruptcies:     %12.0f", bankruptcies);

    if (DIAG_EARLY_WARN && exit_rate / 100.0 > WARN_FIRM_EXIT)
        LOG("\n  >>> WARNING: High firm exit rate! <<<");
}


/*******************************************************************************
 * log_household_identity_check - Wage and profit distribution identity
 ******************************************************************************/
void log_household_identity_check()
{
    if (households == NULL) return;

    // Wage identity: SUM(Household_Wage_Income) = Country_Total_Wages
    double hh_wages = VS(working_class, "Class_Wage_Income") + VS(capitalist_class, "Class_Wage_Income");
    double country_wages = VS(country, "Country_Total_Wages");
    double wage_diff = fabs(hh_wages - country_wages);
    double wage_pct = (country_wages > 0) ? wage_diff / country_wages * 100 : 0;

    // Profit identity: SUM(Household_Profit_Income) = Country_Total_Profits
    double hh_profits = VS(working_class, "Class_Profit_Income") + VS(capitalist_class, "Class_Profit_Income");
    double country_profits = VS(country, "Country_Total_Profits");
    double profit_diff = fabs(hh_profits - country_profits);
    double profit_pct = (country_profits > 0) ? profit_diff / country_profits * 100 : 0;

    char wage_ok = (wage_pct < 0.1) ? 'O' : 'X';
    char profit_ok = (profit_pct < 0.1) ? 'O' : 'X';

    LOG("\n");
    LOG("\n  HOUSEHOLD IDENTITY CHECK");
    LOG("\n  ------------------------");
    LOG("\n  Wages:  HH=%10.2f  Country=%10.2f  [%c]", hh_wages, country_wages, wage_ok);
    LOG("\n  Profits: HH=%10.2f  Country=%10.2f  [%c]", hh_profits, country_profits, profit_ok);

    if (wage_pct >= 0.1)
        LOG("\n  >>> WARNING: Wage identity error (%.2f%%) <<<", wage_pct);
    if (profit_pct >= 0.1)
        LOG("\n  >>> WARNING: Profit identity error (%.2f%%) <<<", profit_pct);
}


/*******************************************************************************
 * log_sectoral_detail - Per-sector breakdown (Verbosity >= 2)
 ******************************************************************************/
void log_sectoral_detail()
{
    LOG("\n");
    LOG("\n  SECTORAL BREAKDOWN");
    LOG("\n  ------------------");
    LOG("\n  %-12s %8s %8s %8s %8s", "Sector", "Firms", "Emp", "CapUtil", "Price");

    object *cur;
    CYCLES(country, cur, "SECTORS")
    {
        const char* sec_name;
        // Sector IDs are parameters
        if (VS(cur, "id_consumption_goods_sector") == 1) sec_name = "Consumption";
        else if (VS(cur, "id_capital_goods_sector") == 1) sec_name = "Capital";
        else sec_name = "Intermediate";

        double firms = COUNTS(cur, "FIRMS");
        double emp = VS(cur, "Sector_Employment");
        double util = VS(cur, "Sector_Capacity_Utilization") * 100;
        double price = VS(cur, "Sector_Avg_Price");

        LOG("\n  %-12s %8.0f %8.0f %7.1f%% %8.4f", sec_name, firms, emp, util, price);
    }
}


/*******************************************************************************
 * log_final_summary - End-of-simulation summary with CAGR
 ******************************************************************************/
void log_final_summary(int t)
{
    // Get final GDP (current period)
    double gdp_final = VS(country, "Country_GDP");
    // Use the existing annual growth variable
    double annual_growth = VS(country, "Country_Annual_Real_Growth");
    double years = (double)t / 4.0;  // Assuming quarterly

    // Get final metrics
    double v_final = VS(country, "Country_Wealth_Capital_Ratio");
    double cap_share_final = VS(country, "Country_Capitalist_Wealth_Share");
    double gini_final = VS(country, "Country_Gini_Index");
    double u_rate_final = VS(country, "Country_Unemployment_Rate");

    LOG("\n");
    LOG("\n  ================================================");
    LOG("\n  ||           SIMULATION COMPLETE              ||");
    LOG("\n  ================================================");
    LOG("\n  Total Periods:    %d (%.1f years)", t, years);
    LOG("\n  GDP (Final):      %.2f", gdp_final);
    LOG("\n  Annual Growth:    %.2f%%", annual_growth * 100);
    LOG("\n  ------------------------------------------------");
    LOG("\n  SECULAR STAGNATION ASSESSMENT:");
    LOG("\n  Valuation Ratio:  %.2f  %s", v_final,
        v_final > 2.0 ? "[FINANCIALIZED]" : "[NORMAL]");
    LOG("\n  Cap Wealth Share: %.1f%%", cap_share_final * 100);
    LOG("\n  Final Gini:       %.3f", gini_final);
    LOG("\n  Final Unemp Rate: %.1f%%", u_rate_final * 100);
    LOG("\n  ================================================");
}
