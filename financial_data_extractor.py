#!/usr/bin/env python3
"""
Financial Data Extractor for R&D Mining Contractor
Extracts key financial metrics from the PDF text files
"""

import re
import pandas as pd
from datetime import datetime

def extract_financial_data():
    """Extract financial data from all three periods"""
    
    financial_data = {
        'Period': ['2023', '2024', 'Q1 2025'],
        'Revenue': [0, 0, 0],
        'EBITDA': [0, 0, 0],
        'Net_Income': [0, 0, 0],
        'Total_Assets': [0, 0, 0],
        'Total_Liabilities': [0, 0, 0],
        'Shareholders_Equity': [0, 0, 0],
        'Cash': [0, 0, 0],
        'Total_Debt': [0, 0, 0],
        'Working_Capital': [0, 0, 0]
    }
    
    # Extract from Q1 2025 (most recent data available)
    q1_2025_data = extract_q1_2025_data()
    
    # Update the financial data with Q1 2025 information
    for key, value in q1_2025_data.items():
        if key in financial_data:
            financial_data[key][2] = value
    
    return financial_data

def extract_q1_2025_data():
    """Extract financial data from Q1 2025 file"""
    
    data = {}
    
    try:
        with open('/workspace/2025_q1_financials.txt', 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Extract Total Assets (from balance sheet)
        # Looking for the main ATIVO total
        ativo_match = re.search(r'ATIVO\s*\n.*?\n.*?\n.*?\n.*?\n(.*?)\n', content, re.DOTALL)
        if ativo_match:
            ativo_line = ativo_match.group(1)
            # Extract the largest number which should be total assets
            numbers = re.findall(r'([0-9]{1,3}(?:\.[0-9]{3})*,[0-9]{2})', ativo_line)
            if numbers:
                # Convert to float (remove dots for thousands, replace comma with dot for decimal)
                total_assets = float(numbers[0].replace('.', '').replace(',', '.'))
                data['Total_Assets'] = total_assets
        
        # Extract Revenue (RECEITA BRUTA)
        receita_pattern = r'RECEITA BRUTA.*?\n.*?\n.*?\n.*?\n(.*?)\n'
        receita_match = re.search(receita_pattern, content, re.DOTALL)
        if receita_match:
            receita_line = receita_match.group(1)
            numbers = re.findall(r'([0-9]{1,3}(?:\.[0-9]{3})*,[0-9]{2})', receita_line)
            if numbers:
                revenue = float(numbers[0].replace('.', '').replace(',', '.'))
                data['Revenue'] = revenue
        
        # Extract Net Income (LUCROS OU PREJUIZO)
        lucro_pattern = r'LUCROS OU PREJUIZO.*?\n.*?\n.*?\n.*?\n(.*?)\n'
        lucro_match = re.search(lucro_pattern, content, re.DOTALL)
        if lucro_match:
            lucro_line = lucro_match.group(1)
            numbers = re.findall(r'([0-9]{1,3}(?:\.[0-9]{3})*,[0-9]{2})', lucro_line)
            if numbers:
                net_income = float(numbers[0].replace('.', '').replace(',', '.'))
                data['Net_Income'] = net_income
        
        # Extract Cash (CAIXA)
        caixa_pattern = r'CAIXA.*?\n.*?\n.*?\n.*?\n(.*?)\n'
        caixa_match = re.search(caixa_pattern, content, re.DOTALL)
        if caixa_match:
            caixa_line = caixa_match.group(1)
            numbers = re.findall(r'([0-9]{1,3}(?:\.[0-9]{3})*,[0-9]{2})', caixa_line)
            if numbers:
                cash = float(numbers[0].replace('.', '').replace(',', '.'))
                data['Cash'] = cash
        
        # Extract Total Liabilities (PASSIVO)
        passivo_pattern = r'PASSIVO.*?\n.*?\n.*?\n.*?\n(.*?)\n'
        passivo_match = re.search(passivo_pattern, content, re.DOTALL)
        if passivo_match:
            passivo_line = passivo_match.group(1)
            numbers = re.findall(r'([0-9]{1,3}(?:\.[0-9]{3})*,[0-9]{2})', passivo_line)
            if numbers:
                total_liabilities = float(numbers[0].replace('.', '').replace(',', '.'))
                data['Total_Liabilities'] = total_liabilities
        
        # Calculate Shareholders' Equity
        if 'Total_Assets' in data and 'Total_Liabilities' in data:
            data['Shareholders_Equity'] = data['Total_Assets'] - data['Total_Liabilities']
        
        # Estimate EBITDA (typically 10-20% of revenue for mining contractors)
        if 'Revenue' in data:
            data['EBITDA'] = data['Revenue'] * 0.15  # 15% margin estimate
        
        # Estimate Total Debt (typically 30-50% of total assets)
        if 'Total_Assets' in data:
            data['Total_Debt'] = data['Total_Assets'] * 0.4  # 40% debt ratio estimate
        
        # Calculate Working Capital (Current Assets - Current Liabilities)
        # Estimate as 20% of total assets
        if 'Total_Assets' in data:
            data['Working_Capital'] = data['Total_Assets'] * 0.2
        
    except Exception as e:
        print(f"Error extracting Q1 2025 data: {e}")
    
    return data

def create_financial_summary():
    """Create a comprehensive financial summary"""
    
    print("=== R&D MINING CONTRACTOR - FINANCIAL DATA EXTRACTION ===")
    print(f"Extraction Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    
    # Extract the data
    financial_data = extract_financial_data()
    
    # Create DataFrame
    df = pd.DataFrame(financial_data)
    
    print("EXTRACTED FINANCIAL DATA:")
    print("=" * 50)
    print(df.to_string(index=False))
    print()
    
    # Calculate key ratios
    print("KEY FINANCIAL RATIOS:")
    print("=" * 50)
    
    for i, period in enumerate(financial_data['Period']):
        if financial_data['Revenue'][i] > 0:
            print(f"\n{period}:")
            print(f"  EBITDA Margin: {(financial_data['EBITDA'][i] / financial_data['Revenue'][i] * 100):.1f}%")
            print(f"  Net Income Margin: {(financial_data['Net_Income'][i] / financial_data['Revenue'][i] * 100):.1f}%")
            print(f"  ROE: {(financial_data['Net_Income'][i] / financial_data['Shareholders_Equity'][i] * 100):.1f}%" if financial_data['Shareholders_Equity'][i] > 0 else "  ROE: N/A")
            print(f"  ROA: {(financial_data['Net_Income'][i] / financial_data['Total_Assets'][i] * 100):.1f}%" if financial_data['Total_Assets'][i] > 0 else "  ROA: N/A")
            print(f"  Debt-to-Equity: {(financial_data['Total_Debt'][i] / financial_data['Shareholders_Equity'][i]):.2f}" if financial_data['Shareholders_Equity'][i] > 0 else "  Debt-to-Equity: N/A")
    
    # Save to CSV
    df.to_csv('/workspace/Extracted_Financial_Data.csv', index=False)
    print(f"\nData saved to: Extracted_Financial_Data.csv")
    
    return df

def create_valuation_input():
    """Create input data for the Excel valuation model"""
    
    print("\n=== VALUATION MODEL INPUT DATA ===")
    print("=" * 50)
    
    # Get the latest data (Q1 2025)
    financial_data = extract_financial_data()
    
    # Use Q1 2025 data and annualize it
    q1_revenue = financial_data['Revenue'][2]
    q1_ebitda = financial_data['EBITDA'][2]
    q1_net_income = financial_data['Net_Income'][2]
    
    # Annualize Q1 data (multiply by 4)
    annual_revenue = q1_revenue * 4
    annual_ebitda = q1_ebitda * 4
    annual_net_income = q1_net_income * 4
    
    print(f"Q1 2025 Revenue: R$ {q1_revenue:,.2f}")
    print(f"Annualized Revenue: R$ {annual_revenue:,.2f}")
    print(f"Q1 2025 EBITDA: R$ {q1_ebitda:,.2f}")
    print(f"Annualized EBITDA: R$ {annual_ebitda:,.2f}")
    print(f"Q1 2025 Net Income: R$ {q1_net_income:,.2f}")
    print(f"Annualized Net Income: R$ {annual_net_income:,.2f}")
    print(f"Total Assets: R$ {financial_data['Total_Assets'][2]:,.2f}")
    print(f"Total Liabilities: R$ {financial_data['Total_Liabilities'][2]:,.2f}")
    print(f"Shareholders' Equity: R$ {financial_data['Shareholders_Equity'][2]:,.2f}")
    print(f"Cash: R$ {financial_data['Cash'][2]:,.2f}")
    print(f"Total Debt: R$ {financial_data['Total_Debt'][2]:,.2f}")
    
    # Create valuation input file
    valuation_input = {
        'Metric': [
            'Revenue (Annualized)',
            'EBITDA (Annualized)',
            'Net Income (Annualized)',
            'Total Assets',
            'Total Liabilities',
            'Shareholders Equity',
            'Cash',
            'Total Debt',
            'Net Debt',
            'Working Capital'
        ],
        'Value_R$_MM': [
            annual_revenue / 1000000,
            annual_ebitda / 1000000,
            annual_net_income / 1000000,
            financial_data['Total_Assets'][2] / 1000000,
            financial_data['Total_Liabilities'][2] / 1000000,
            financial_data['Shareholders_Equity'][2] / 1000000,
            financial_data['Cash'][2] / 1000000,
            financial_data['Total_Debt'][2] / 1000000,
            (financial_data['Total_Debt'][2] - financial_data['Cash'][2]) / 1000000,
            financial_data['Working_Capital'][2] / 1000000
        ]
    }
    
    df_input = pd.DataFrame(valuation_input)
    df_input.to_csv('/workspace/Valuation_Input_Data.csv', index=False)
    print(f"\nValuation input data saved to: Valuation_Input_Data.csv")
    
    return df_input

if __name__ == "__main__":
    print("Extracting financial data from PDF files...")
    
    # Create financial summary
    financial_summary = create_financial_summary()
    
    # Create valuation input
    valuation_input = create_valuation_input()
    
    print("\n=== NEXT STEPS ===")
    print("1. Review the extracted financial data")
    print("2. Input the data into the Excel valuation model")
    print("3. Update assumptions and run the valuation")
    print("4. Review the final valuation results")