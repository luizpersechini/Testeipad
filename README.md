# Mining Contractor Valuation Model

## Overview
This repository contains a comprehensive Excel valuation model for R&D Mining Contractor, created using standard financial valuation methodologies. The model includes multiple valuation approaches to provide a robust assessment of the company's value.

## Files Created

### 1. Main Valuation Model
- **`Mining_Contractor_Valuation_Model.xlsx`** - Comprehensive Excel valuation model with 10 sheets:
  - Executive Summary
  - Financial Data Input
  - Historical Analysis
  - DCF Valuation
  - Comparable Company Analysis
  - Precedent Transactions
  - Asset-Based Valuation
  - Valuation Summary
  - Sensitivity Analysis
  - Risk Analysis

### 2. Supporting Documents
- **`Valuation_Model_Guide.md`** - Detailed user guide with step-by-step instructions
- **`Financial_Data_Extraction_Template.csv`** - Template for extracting data from PDF statements
- **`pdf_data_extractor.py`** - Python script for analysis and sample calculations

### 3. Source Financial Statements
- **`R&D - Demonstrações financeiras 2023.pdf`** (519 KB)
- **`R&D - Demonstrações financeiras 2024.pdf`** (781 KB)
- **`R&D - Demonstrações financeiras Q1 2025.pdf`** (349 KB)

## Valuation Methodologies Included

### 1. Discounted Cash Flow (DCF)
- 5-year financial projections
- Terminal value calculation
- WACC-based discounting
- Sensitivity analysis for key assumptions

### 2. Comparable Company Analysis
- Trading multiples (P/E, EV/EBITDA, EV/Revenue, P/B)
- Peer company benchmarking
- Market-based valuation

### 3. Precedent Transactions
- Recent M&A transaction multiples
- Market premium analysis
- Transaction-based valuation

### 4. Asset-Based Valuation
- Book value vs. market value
- Liquidation value analysis
- Going concern adjustments

## Key Features

### Financial Analysis
- Historical financial performance analysis
- Key financial ratios and metrics
- Growth rate calculations
- Profitability and efficiency analysis

### Risk Assessment
- Comprehensive risk identification
- Risk impact and probability assessment
- Mitigation strategies
- Risk-adjusted valuation considerations

### Sensitivity Analysis
- WACC sensitivity tables
- Growth rate scenarios
- Multiple scenario analysis (base, optimistic, pessimistic)

## How to Use

### Step 1: Extract Financial Data
1. Open the PDF financial statements
2. Extract key financial metrics for each year
3. Use the `Financial_Data_Extraction_Template.csv` to organize data

### Step 2: Input Data into Model
1. Open `Mining_Contractor_Valuation_Model.xlsx`
2. Go to "Financial Data Input" sheet
3. Enter the extracted financial data
4. The model will automatically calculate ratios and metrics

### Step 3: Update Assumptions
1. Review and update DCF assumptions (WACC, growth rates, tax rate)
2. Research and input comparable company data
3. Update precedent transaction information
4. Adjust asset valuations as needed

### Step 4: Review Results
1. Check "Valuation Summary" for final results
2. Review sensitivity analysis
3. Analyze risk factors
4. Prepare executive summary

## Typical Mining Contractor Valuation Ranges

| Metric | Typical Range |
|--------|---------------|
| P/E Ratio | 8-15x |
| EV/EBITDA | 6-12x |
| EV/Revenue | 0.8-2.0x |
| P/B Ratio | 1.0-2.5x |

## Key Assumptions

- **WACC**: 10-15% (typical for mining contractors)
- **Terminal Growth Rate**: 2-3% (long-term GDP growth)
- **Tax Rate**: 25% (Brazil corporate tax rate)
- **Revenue Growth**: Based on historical trends and market outlook

## Industry-Specific Considerations

### Mining Contractor Characteristics
- Cyclical business tied to commodity prices
- Capital intensive with high equipment costs
- Project-based revenue dependent on contract wins
- Often geographically concentrated

### Key Value Drivers
1. Contract backlog and future revenue visibility
2. Equipment fleet age, condition, and utilization
3. Client relationships and long-term contracts
4. Operational efficiency and cost management
5. Market position and competitive advantages

### Major Risk Factors
1. Commodity price volatility
2. Economic cycles affecting mining investment
3. Regulatory changes (environmental, safety)
4. Intense competition for contracts
5. Currency risk (if multi-country operations)

## Next Steps

1. **Extract Data**: Manually extract financial data from PDF statements
2. **Research Comparables**: Find similar mining contractors and recent transactions
3. **Validate Assumptions**: Ensure all assumptions are realistic and well-supported
4. **Sensitivity Analysis**: Test key assumptions and scenarios
5. **Final Review**: Cross-check calculations and prepare presentation

## Technical Requirements

- Microsoft Excel (2016 or later recommended)
- Python 3.x (for data extraction script)
- pandas and openpyxl libraries (for Excel generation)

## Support and Documentation

- See `Valuation_Model_Guide.md` for detailed instructions
- Use `Financial_Data_Extraction_Template.csv` for data organization
- Run `pdf_data_extractor.py` for analysis and sample calculations

## Disclaimer

This valuation model is a template and framework. All financial data, assumptions, and calculations should be verified and validated by qualified financial professionals. The model provides a starting point for valuation analysis but should not be used as the sole basis for investment decisions.

---

**Created**: September 11, 2025  
**Company**: R&D Mining Contractor  
**Model Type**: Comprehensive Multi-Method Valuation  
**Currency**: Brazilian Real (R$)