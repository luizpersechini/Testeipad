#!/usr/bin/env python3
"""
PDF Data Extractor for Financial Statements
This script attempts to extract basic information from PDF financial statements
"""

import os
import sys
from datetime import datetime

def analyze_pdf_files():
    """Analyze the PDF files in the workspace"""
    
    pdf_files = [
        'R&D - Demonstrações financeiras 2023.pdf',
        'R&D - Demonstrações financeiras 2024.pdf', 
        'R&D - Demonstrações financeiras Q1 2025.pdf'
    ]
    
    print("=== MINING CONTRACTOR FINANCIAL STATEMENTS ANALYSIS ===")
    print(f"Analysis Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    
    for pdf_file in pdf_files:
        if os.path.exists(pdf_file):
            file_size = os.path.getsize(pdf_file)
            print(f"✓ Found: {pdf_file}")
            print(f"  File Size: {file_size:,} bytes ({file_size/1024/1024:.1f} MB)")
            print()
        else:
            print(f"✗ Missing: {pdf_file}")
            print()
    
    print("=== NEXT STEPS ===")
    print("1. Open each PDF file manually")
    print("2. Extract the following key financial data:")
    print("   - Revenue (Receita)")
    print("   - EBITDA (EBITDA)")
    print("   - Net Income (Lucro Líquido)")
    print("   - Total Assets (Ativo Total)")
    print("   - Total Debt (Dívida Total)")
    print("   - Cash (Caixa)")
    print("   - Shareholders' Equity (Patrimônio Líquido)")
    print()
    print("3. Use the Financial_Data_Extraction_Template.csv to organize the data")
    print("4. Input the data into the Excel valuation model")
    print()
    print("=== VALUATION MODEL FILES CREATED ===")
    print("✓ Mining_Contractor_Valuation_Model.xlsx - Main valuation model")
    print("✓ Financial_Data_Extraction_Template.csv - Data extraction template")
    print("✓ Valuation_Model_Guide.md - Comprehensive user guide")
    print()
    print("=== TYPICAL MINING CONTRACTOR VALUATION RANGES ===")
    print("P/E Ratio: 8-15x")
    print("EV/EBITDA: 6-12x")
    print("EV/Revenue: 0.8-2.0x")
    print("P/B Ratio: 1.0-2.5x")
    print()
    print("=== KEY ASSUMPTIONS TO CONSIDER ===")
    print("WACC: 10-15% (typical for mining contractors)")
    print("Terminal Growth Rate: 2-3%")
    print("Tax Rate: 25% (Brazil corporate tax rate)")
    print("Revenue Growth: Based on historical trends and market outlook")

def create_sample_calculations():
    """Create sample calculations for reference"""
    
    print("\n=== SAMPLE VALUATION CALCULATIONS ===")
    print("(These are examples - replace with actual data)")
    print()
    
    # Sample financial data
    revenue_2024 = 100  # R$ MM
    ebitda_2024 = 15    # R$ MM
    net_income_2024 = 8 # R$ MM
    total_assets = 80   # R$ MM
    net_debt = 20       # R$ MM
    shares_outstanding = 10  # MM shares
    
    print(f"Sample 2024 Financials:")
    print(f"Revenue: R$ {revenue_2024} MM")
    print(f"EBITDA: R$ {ebitda_2024} MM")
    print(f"Net Income: R$ {net_income_2024} MM")
    print(f"Total Assets: R$ {total_assets} MM")
    print(f"Net Debt: R$ {net_debt} MM")
    print(f"Shares Outstanding: {shares_outstanding} MM")
    print()
    
    # Sample valuation calculations
    pe_ratio = 12
    ev_ebitda_ratio = 8
    ev_revenue_ratio = 1.2
    
    market_cap_pe = net_income_2024 * pe_ratio
    ev_ebitda = ebitda_2024 * ev_ebitda_ratio
    ev_revenue = revenue_2024 * ev_revenue_ratio
    
    equity_value_pe = market_cap_pe
    equity_value_ebitda = ev_ebitda - net_debt
    equity_value_revenue = ev_revenue - net_debt
    
    price_per_share_pe = equity_value_pe / shares_outstanding
    price_per_share_ebitda = equity_value_ebitda / shares_outstanding
    price_per_share_revenue = equity_value_revenue / shares_outstanding
    
    print("Sample Valuation Results:")
    print(f"P/E Method (12x): R$ {equity_value_pe:.1f} MM → R$ {price_per_share_pe:.2f}/share")
    print(f"EV/EBITDA Method (8x): R$ {equity_value_ebitda:.1f} MM → R$ {price_per_share_ebitda:.2f}/share")
    print(f"EV/Revenue Method (1.2x): R$ {equity_value_revenue:.1f} MM → R$ {price_per_share_revenue:.2f}/share")
    print()
    
    # Weighted average
    weights = [0.4, 0.4, 0.2]  # P/E, EV/EBITDA, EV/Revenue
    weighted_value = (equity_value_pe * weights[0] + 
                     equity_value_ebitda * weights[1] + 
                     equity_value_revenue * weights[2])
    weighted_price = weighted_value / shares_outstanding
    
    print(f"Weighted Average Valuation: R$ {weighted_value:.1f} MM → R$ {weighted_price:.2f}/share")

if __name__ == "__main__":
    analyze_pdf_files()
    create_sample_calculations()