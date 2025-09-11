#!/usr/bin/env python3
"""
Mining Contractor Valuation Model
Creates a comprehensive Excel valuation model for mining contractors
"""

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import warnings
warnings.filterwarnings('ignore')

def create_valuation_model():
    """Create a comprehensive Excel valuation model for mining contractors"""
    
    # Create Excel writer
    with pd.ExcelWriter('/workspace/Mining_Contractor_Valuation_Model.xlsx', engine='openpyxl') as writer:
        
        # 1. EXECUTIVE SUMMARY
        create_executive_summary(writer)
        
        # 2. FINANCIAL DATA INPUT
        create_financial_data_input(writer)
        
        # 3. HISTORICAL FINANCIAL ANALYSIS
        create_historical_analysis(writer)
        
        # 4. DCF VALUATION
        create_dcf_valuation(writer)
        
        # 5. COMPARABLE COMPANY ANALYSIS
        create_comparable_analysis(writer)
        
        # 6. PRECEDENT TRANSACTIONS
        create_precedent_transactions(writer)
        
        # 7. ASSET-BASED VALUATION
        create_asset_based_valuation(writer)
        
        # 8. VALUATION SUMMARY
        create_valuation_summary(writer)
        
        # 9. SENSITIVITY ANALYSIS
        create_sensitivity_analysis(writer)
        
        # 10. RISK ANALYSIS
        create_risk_analysis(writer)

def create_executive_summary(writer):
    """Create executive summary sheet"""
    summary_data = {
        'Metric': [
            'Company Name',
            'Valuation Date',
            'Business Description',
            'Primary Services',
            'Geographic Focus',
            'Key Clients',
            'Total Revenue (Latest Year)',
            'EBITDA (Latest Year)',
            'Net Income (Latest Year)',
            'Total Assets',
            'Total Debt',
            'Cash & Equivalents',
            'DCF Valuation',
            'Comparable Company Valuation',
            'Precedent Transaction Valuation',
            'Asset-Based Valuation',
            'Final Valuation Range',
            'Key Assumptions',
            'Major Risks',
            'Recommendations'
        ],
        'Value': [
            'R&D Mining Contractor',
            datetime.now().strftime('%Y-%m-%d'),
            'Mining contractor providing specialized services',
            'Mining operations, equipment rental, consulting',
            'Brazil (Primary)',
            'Major mining companies',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED]',
            'R$ [TO BE FILLED] - R$ [TO BE FILLED]',
            'See detailed assumptions in model',
            'See risk analysis sheet',
            'See recommendations in summary'
        ]
    }
    
    df = pd.DataFrame(summary_data)
    df.to_excel(writer, sheet_name='Executive Summary', index=False)

def create_financial_data_input(writer):
    """Create financial data input sheet"""
    
    # Income Statement Template
    income_statement = {
        'Year': ['2021', '2022', '2023', '2024', 'Q1 2025 (Annualized)'],
        'Revenue': [0, 0, 0, 0, 0],
        'Cost of Goods Sold': [0, 0, 0, 0, 0],
        'Gross Profit': [0, 0, 0, 0, 0],
        'Operating Expenses': [0, 0, 0, 0, 0],
        'EBITDA': [0, 0, 0, 0, 0],
        'Depreciation & Amortization': [0, 0, 0, 0, 0],
        'EBIT': [0, 0, 0, 0, 0],
        'Interest Expense': [0, 0, 0, 0, 0],
        'EBT': [0, 0, 0, 0, 0],
        'Tax Expense': [0, 0, 0, 0, 0],
        'Net Income': [0, 0, 0, 0, 0]
    }
    
    # Balance Sheet Template
    balance_sheet = {
        'Year': ['2021', '2022', '2023', '2024', 'Q1 2025'],
        'Cash & Equivalents': [0, 0, 0, 0, 0],
        'Accounts Receivable': [0, 0, 0, 0, 0],
        'Inventory': [0, 0, 0, 0, 0],
        'Other Current Assets': [0, 0, 0, 0, 0],
        'Total Current Assets': [0, 0, 0, 0, 0],
        'PP&E (Net)': [0, 0, 0, 0, 0],
        'Intangible Assets': [0, 0, 0, 0, 0],
        'Other Assets': [0, 0, 0, 0, 0],
        'Total Assets': [0, 0, 0, 0, 0],
        'Accounts Payable': [0, 0, 0, 0, 0],
        'Short-term Debt': [0, 0, 0, 0, 0],
        'Other Current Liabilities': [0, 0, 0, 0, 0],
        'Total Current Liabilities': [0, 0, 0, 0, 0],
        'Long-term Debt': [0, 0, 0, 0, 0],
        'Other Liabilities': [0, 0, 0, 0, 0],
        'Total Liabilities': [0, 0, 0, 0, 0],
        'Shareholders Equity': [0, 0, 0, 0, 0]
    }
    
    # Cash Flow Statement Template
    cash_flow = {
        'Year': ['2021', '2022', '2023', '2024', 'Q1 2025'],
        'Net Income': [0, 0, 0, 0, 0],
        'Depreciation & Amortization': [0, 0, 0, 0, 0],
        'Working Capital Changes': [0, 0, 0, 0, 0],
        'Operating Cash Flow': [0, 0, 0, 0, 0],
        'Capital Expenditures': [0, 0, 0, 0, 0],
        'Free Cash Flow': [0, 0, 0, 0, 0],
        'Debt Issuance/(Repayment)': [0, 0, 0, 0, 0],
        'Dividends Paid': [0, 0, 0, 0, 0],
        'Net Cash Flow': [0, 0, 0, 0, 0]
    }
    
    # Create DataFrames and write to Excel
    pd.DataFrame(income_statement).to_excel(writer, sheet_name='Financial Data Input', index=False, startrow=0)
    pd.DataFrame(balance_sheet).to_excel(writer, sheet_name='Financial Data Input', index=False, startrow=15)
    pd.DataFrame(cash_flow).to_excel(writer, sheet_name='Financial Data Input', index=False, startrow=35)

def create_historical_analysis(writer):
    """Create historical financial analysis sheet"""
    
    analysis_data = {
        'Metric': [
            'Revenue Growth Rate (YoY)',
            'EBITDA Margin',
            'EBIT Margin',
            'Net Income Margin',
            'ROE (Return on Equity)',
            'ROA (Return on Assets)',
            'ROIC (Return on Invested Capital)',
            'Current Ratio',
            'Quick Ratio',
            'Debt-to-Equity Ratio',
            'Interest Coverage Ratio',
            'Asset Turnover',
            'Inventory Turnover',
            'Receivables Turnover',
            'Free Cash Flow Margin',
            'Capex as % of Revenue',
            'Working Capital Days',
            'Cash Conversion Cycle'
        ],
        '2021': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        '2022': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        '2023': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        '2024': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        'Average': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    }
    
    df = pd.DataFrame(analysis_data)
    df.to_excel(writer, sheet_name='Historical Analysis', index=False)

def create_dcf_valuation(writer):
    """Create DCF valuation sheet"""
    
    # DCF Assumptions
    dcf_assumptions = {
        'Assumption': [
            'Terminal Growth Rate (%)',
            'WACC (%)',
            'Tax Rate (%)',
            'Revenue Growth Rate (Years 1-3)',
            'Revenue Growth Rate (Years 4-5)',
            'EBITDA Margin (Terminal)',
            'Capex as % of Revenue',
            'Working Capital as % of Revenue',
            'Depreciation as % of Capex'
        ],
        'Value': [2.5, 12.0, 25.0, 8.0, 5.0, 15.0, 8.0, 10.0, 80.0]
    }
    
    # DCF Projections (5 years)
    years = ['2025', '2026', '2027', '2028', '2029', 'Terminal']
    dcf_projections = {
        'Year': years,
        'Revenue': [0, 0, 0, 0, 0, 0],
        'EBITDA': [0, 0, 0, 0, 0, 0],
        'EBIT': [0, 0, 0, 0, 0, 0],
        'Taxes': [0, 0, 0, 0, 0, 0],
        'NOPAT': [0, 0, 0, 0, 0, 0],
        'Depreciation': [0, 0, 0, 0, 0, 0],
        'Capex': [0, 0, 0, 0, 0, 0],
        'Working Capital Change': [0, 0, 0, 0, 0, 0],
        'Free Cash Flow': [0, 0, 0, 0, 0, 0],
        'Discount Factor': [0, 0, 0, 0, 0, 0],
        'Present Value': [0, 0, 0, 0, 0, 0]
    }
    
    # DCF Summary
    dcf_summary = {
        'Metric': [
            'Sum of PV of FCF (2025-2029)',
            'Terminal Value',
            'PV of Terminal Value',
            'Enterprise Value',
            'Less: Net Debt',
            'Equity Value',
            'Shares Outstanding',
            'Value per Share'
        ],
        'Value': [0, 0, 0, 0, 0, 0, 0, 0]
    }
    
    # Write to Excel
    pd.DataFrame(dcf_assumptions).to_excel(writer, sheet_name='DCF Valuation', index=False, startrow=0)
    pd.DataFrame(dcf_projections).to_excel(writer, sheet_name='DCF Valuation', index=False, startrow=12)
    pd.DataFrame(dcf_summary).to_excel(writer, sheet_name='DCF Valuation', index=False, startrow=25)

def create_comparable_analysis(writer):
    """Create comparable company analysis sheet"""
    
    # Comparable Companies Template
    comparable_companies = {
        'Company': [
            'Vale S.A.',
            'Petrobras',
            'Usiminas',
            'Gerdau',
            'CSN',
            'CEMIG',
            'Average',
            'Median'
        ],
        'Market Cap (R$ MM)': [0, 0, 0, 0, 0, 0, 0, 0],
        'Enterprise Value (R$ MM)': [0, 0, 0, 0, 0, 0, 0, 0],
        'Revenue (R$ MM)': [0, 0, 0, 0, 0, 0, 0, 0],
        'EBITDA (R$ MM)': [0, 0, 0, 0, 0, 0, 0, 0],
        'Net Income (R$ MM)': [0, 0, 0, 0, 0, 0, 0, 0],
        'P/E Ratio': [0, 0, 0, 0, 0, 0, 0, 0],
        'EV/Revenue': [0, 0, 0, 0, 0, 0, 0, 0],
        'EV/EBITDA': [0, 0, 0, 0, 0, 0, 0, 0],
        'P/B Ratio': [0, 0, 0, 0, 0, 0, 0, 0]
    }
    
    # Valuation based on comparables
    comparable_valuation = {
        'Method': [
            'P/E Multiple',
            'EV/Revenue Multiple',
            'EV/EBITDA Multiple',
            'P/B Multiple'
        ],
        'Multiple': [0, 0, 0, 0],
        'Target Company Metric': [0, 0, 0, 0],
        'Implied Value': [0, 0, 0, 0]
    }
    
    # Write to Excel
    pd.DataFrame(comparable_companies).to_excel(writer, sheet_name='Comparable Analysis', index=False, startrow=0)
    pd.DataFrame(comparable_valuation).to_excel(writer, sheet_name='Comparable Analysis', index=False, startrow=12)

def create_precedent_transactions(writer):
    """Create precedent transactions analysis sheet"""
    
    precedent_transactions = {
        'Transaction': [
            'Transaction 1',
            'Transaction 2',
            'Transaction 3',
            'Transaction 4',
            'Transaction 5',
            'Average',
            'Median'
        ],
        'Date': ['2023-01-01', '2023-06-01', '2024-01-01', '2024-06-01', '2024-12-01', '', ''],
        'Target Company': ['Company A', 'Company B', 'Company C', 'Company D', 'Company E', '', ''],
        'Acquirer': ['Acquirer A', 'Acquirer B', 'Acquirer C', 'Acquirer D', 'Acquirer E', '', ''],
        'Transaction Value (R$ MM)': [0, 0, 0, 0, 0, 0, 0],
        'Revenue (R$ MM)': [0, 0, 0, 0, 0, 0, 0],
        'EBITDA (R$ MM)': [0, 0, 0, 0, 0, 0, 0],
        'EV/Revenue': [0, 0, 0, 0, 0, 0, 0],
        'EV/EBITDA': [0, 0, 0, 0, 0, 0, 0]
    }
    
    # Valuation based on precedent transactions
    precedent_valuation = {
        'Method': [
            'EV/Revenue Multiple',
            'EV/EBITDA Multiple'
        ],
        'Multiple': [0, 0],
        'Target Company Metric': [0, 0],
        'Implied Value': [0, 0]
    }
    
    # Write to Excel
    pd.DataFrame(precedent_transactions).to_excel(writer, sheet_name='Precedent Transactions', index=False, startrow=0)
    pd.DataFrame(precedent_valuation).to_excel(writer, sheet_name='Precedent Transactions', index=False, startrow=12)

def create_asset_based_valuation(writer):
    """Create asset-based valuation sheet"""
    
    asset_valuation = {
        'Asset Category': [
            'Cash & Equivalents',
            'Accounts Receivable',
            'Inventory',
            'PP&E (Book Value)',
            'PP&E (Market Value)',
            'Intangible Assets',
            'Other Assets',
            'Total Assets',
            'Less: Total Liabilities',
            'Net Asset Value',
            'Adjustments',
            'Adjusted Net Asset Value'
        ],
        'Book Value (R$ MM)': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        'Market Value (R$ MM)': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        'Notes': [
            'At face value',
            'Net of allowances',
            'At lower of cost or market',
            'Historical cost less depreciation',
            'Independent appraisal required',
            'Goodwill and other intangibles',
            'Other non-current assets',
            'Sum of all assets',
            'All liabilities',
            'Assets minus liabilities',
            'Market adjustments',
            'Final adjusted value'
        ]
    }
    
    df = pd.DataFrame(asset_valuation)
    df.to_excel(writer, sheet_name='Asset-Based Valuation', index=False)

def create_valuation_summary(writer):
    """Create valuation summary sheet"""
    
    valuation_summary = {
        'Valuation Method': [
            'DCF Valuation',
            'Comparable Company Analysis',
            'Precedent Transactions',
            'Asset-Based Valuation',
            'Weighted Average'
        ],
        'Value (R$ MM)': [0, 0, 0, 0, 0],
        'Weight (%)': [40, 30, 20, 10, 100],
        'Weighted Value (R$ MM)': [0, 0, 0, 0, 0],
        'Notes': [
            'Based on 5-year projections',
            'Based on trading multiples',
            'Based on transaction multiples',
            'Based on asset values',
            'Final weighted valuation'
        ]
    }
    
    # Additional metrics
    additional_metrics = {
        'Metric': [
            'Final Enterprise Value',
            'Less: Net Debt',
            'Equity Value',
            'Shares Outstanding (MM)',
            'Value per Share (R$)',
            'Current Share Price (R$)',
            'Upside/Downside (%)',
            '52-Week High (R$)',
            '52-Week Low (R$)',
            'Valuation Date'
        ],
        'Value': [0, 0, 0, 0, 0, 0, 0, 0, 0, datetime.now().strftime('%Y-%m-%d')]
    }
    
    # Write to Excel
    pd.DataFrame(valuation_summary).to_excel(writer, sheet_name='Valuation Summary', index=False, startrow=0)
    pd.DataFrame(additional_metrics).to_excel(writer, sheet_name='Valuation Summary', index=False, startrow=10)

def create_sensitivity_analysis(writer):
    """Create sensitivity analysis sheet"""
    
    # WACC Sensitivity
    wacc_scenarios = [8, 10, 12, 14, 16]
    growth_scenarios = [1, 2, 2.5, 3, 4]
    
    sensitivity_data = []
    for wacc in wacc_scenarios:
        for growth in growth_scenarios:
            sensitivity_data.append({
                'WACC (%)': wacc,
                'Terminal Growth (%)': growth,
                'DCF Value (R$ MM)': 0  # To be calculated
            })
    
    df_sensitivity = pd.DataFrame(sensitivity_data)
    df_sensitivity.to_excel(writer, sheet_name='Sensitivity Analysis', index=False)

def create_risk_analysis(writer):
    """Create risk analysis sheet"""
    
    risk_analysis = {
        'Risk Category': [
            'Market Risk',
            'Operational Risk',
            'Financial Risk',
            'Regulatory Risk',
            'Environmental Risk',
            'Technology Risk',
            'Competition Risk',
            'Currency Risk',
            'Interest Rate Risk',
            'Credit Risk'
        ],
        'Risk Description': [
            'Fluctuations in commodity prices',
            'Equipment breakdowns, safety incidents',
            'High debt levels, liquidity constraints',
            'Changes in mining regulations',
            'Environmental compliance costs',
            'Technological obsolescence',
            'Intense competition in mining services',
            'Brazilian Real volatility',
            'Rising interest rates',
            'Customer payment delays'
        ],
        'Impact': [
            'High',
            'Medium',
            'High',
            'Medium',
            'Medium',
            'Low',
            'High',
            'Medium',
            'Medium',
            'Medium'
        ],
        'Probability': [
            'High',
            'Medium',
            'Medium',
            'Low',
            'Medium',
            'Low',
            'High',
            'High',
            'Medium',
            'Medium'
        ],
        'Mitigation': [
            'Diversified client base, hedging',
            'Maintenance programs, safety protocols',
            'Debt reduction, cash management',
            'Regulatory monitoring, compliance',
            'Environmental management systems',
            'Technology upgrades, R&D',
            'Service differentiation, cost leadership',
            'Currency hedging, local operations',
            'Fixed-rate debt, interest rate swaps',
            'Credit insurance, payment terms'
        ]
    }
    
    df = pd.DataFrame(risk_analysis)
    df.to_excel(writer, sheet_name='Risk Analysis', index=False)

if __name__ == "__main__":
    print("Creating Mining Contractor Valuation Model...")
    create_valuation_model()
    print("Valuation model created successfully!")
    print("File saved as: Mining_Contractor_Valuation_Model.xlsx")
    print("\nNext steps:")
    print("1. Extract financial data from the PDF statements")
    print("2. Input the data into the 'Financial Data Input' sheet")
    print("3. Update assumptions in the DCF and other valuation sheets")
    print("4. Review and adjust the final valuation")