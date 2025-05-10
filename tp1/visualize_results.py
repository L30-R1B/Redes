import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from datetime import datetime
import numpy as np

# Configuração do estilo dos gráficos (ATUALIZADO)
sns.set_theme(style="whitegrid")  # Novo estilo do Seaborn
plt.rcParams['figure.figsize'] = (12, 6)
plt.rcParams['font.size'] = 12
plt.rcParams['axes.labelsize'] = 12
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['xtick.labelsize'] = 10
plt.rcParams['ytick.labelsize'] = 10

def load_data(results_dir):
    """Carrega os dados dos arquivos CSV de resultados"""
    tcp_file = os.path.join(results_dir, 'tcp_results.csv')
    udp_file = os.path.join(results_dir, 'udp_results.csv')
    
    tcp_df = pd.read_csv(tcp_file)
    udp_df = pd.read_csv(udp_file)
    
    # Adiciona coluna de protocolo
    tcp_df['protocol'] = 'TCP'
    udp_df['protocol'] = 'UDP'
    
    # Combina os dataframes
    combined_df = pd.concat([tcp_df, udp_df])
    
    # Converte tamanho para KB
    combined_df['file_size_kb'] = combined_df['file_size'] / 1024
    
    return combined_df

def generate_comparison_plots(df, output_dir):
    """Gera gráficos comparativos entre TCP e UDP"""
    
    # Média por tamanho de arquivo e protocolo
    avg_df = df.groupby(['file_size', 'protocol']).agg({
        'transfer_time': 'mean',
        'speed_kbs': 'mean',
        'packets_received': 'mean',
        'packet_loss': 'mean'
    }).reset_index()
    
    # Gráfico 1: Tempo de transferência por tamanho de arquivo
    plt.figure(figsize=(14, 7))
    sns.lineplot(data=avg_df, x='file_size', y='transfer_time', hue='protocol', 
                 marker='o', linewidth=2.5)
    plt.title('Tempo Médio de Transferência por Tamanho de Arquivo')
    plt.xlabel('Tamanho do Arquivo (bytes)')
    plt.ylabel('Tempo (segundos)')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(title='Protocolo')
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'transfer_time_comparison.png'), dpi=300)
    plt.close()
    
    # Gráfico 2: Velocidade de transferência por tamanho de arquivo
    plt.figure(figsize=(14, 7))
    sns.lineplot(data=avg_df, x='file_size', y='speed_kbs', hue='protocol', 
                 marker='o', linewidth=2.5)
    plt.title('Velocidade Média de Transferência por Tamanho de Arquivo')
    plt.xlabel('Tamanho do Arquivo (bytes)')
    plt.ylabel('Velocidade (KB/s)')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(title='Protocolo')
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'transfer_speed_comparison.png'), dpi=300)
    plt.close()
    
    # Gráfico 3: Perda de pacotes (UDP apenas)
    if 'packet_loss' in df.columns:
        udp_avg = avg_df[avg_df['protocol'] == 'UDP']
        plt.figure(figsize=(14, 7))
        sns.barplot(data=udp_avg, x='file_size', y='packet_loss')
        plt.title('Perda Média de Pacotes UDP por Tamanho de Arquivo')
        plt.xlabel('Tamanho do Arquivo (bytes)')
        plt.ylabel('Perda de Pacotes (%)')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'udp_packet_loss.png'), dpi=300)
        plt.close()

def generate_distribution_plots(df, output_dir):
    """Gera gráficos de distribuição dos resultados"""
    
    # Gráfico 4: Distribuição das velocidades
    plt.figure(figsize=(14, 7))
    sns.boxplot(data=df, x='file_size', y='speed_kbs', hue='protocol')
    plt.title('Distribuição das Velocidades de Transferência')
    plt.xlabel('Tamanho do Arquivo (bytes)')
    plt.ylabel('Velocidade (KB/s)')
    plt.legend(title='Protocolo')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'speed_distribution.png'), dpi=300)
    plt.close()
    
    # Gráfico 5: Relação entre tamanho e velocidade
    plt.figure(figsize=(14, 7))
    sns.scatterplot(data=df, x='file_size', y='speed_kbs', hue='protocol', 
                    alpha=0.6, s=100)
    plt.title('Relação entre Tamanho do Arquivo e Velocidade de Transferência')
    plt.xlabel('Tamanho do Arquivo (bytes)')
    plt.ylabel('Velocidade (KB/s)')
    plt.legend(title='Protocolo')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'size_vs_speed.png'), dpi=300)
    plt.close()

def generate_3d_plot(df, output_dir):
    """Gera um gráfico 3D interativo (salva como HTML)"""
    try:
        from mpl_toolkits.mplot3d import Axes3D
        import plotly.express as px
        
        # Prepara os dados para plotagem 3D
        plot_df = df.groupby(['file_size', 'protocol']).agg({
            'transfer_time': 'mean',
            'speed_kbs': 'mean',
            'packet_loss': 'mean'
        }).reset_index()
        
        # Cria gráfico 3D interativo
        fig = px.scatter_3d(plot_df, 
                           x='file_size', 
                           y='transfer_time', 
                           z='speed_kbs',
                           color='protocol',
                           size='file_size',
                           hover_name='protocol',
                           title='Relação 3D: Tamanho, Tempo e Velocidade')
        
        # Salva como HTML
        fig.write_html(os.path.join(output_dir, '3d_interactive_plot.html'))
    except ImportError:
        print("Bibliotecas para gráfico 3D não disponíveis. Pulando esta visualização.")

def generate_all_plots(results_dir):
    """Gera todos os gráficos a partir dos dados"""
    # Cria diretório para os gráficos
    plots_dir = os.path.join(results_dir, 'visualizations')
    os.makedirs(plots_dir, exist_ok=True)
    
    # Carrega os dados
    df = load_data(results_dir)
    
    # Gera os gráficos
    generate_comparison_plots(df, plots_dir)
    generate_distribution_plots(df, plots_dir)
    generate_3d_plot(df, plots_dir)
    
    # Cria um relatório HTML com todos os gráficos
    create_html_report(plots_dir, results_dir)

def create_html_report(plots_dir, results_dir):
    """Cria um relatório HTML com todos os gráficos"""
    html_content = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Relatório de Testes TCP vs UDP</title>
        <style>
            body {{ font-family: Arial, sans-serif; margin: 20px; }}
            h1 {{ color: #2c3e50; }}
            h2 {{ color: #3498db; margin-top: 30px; }}
            .plot {{ margin: 20px 0; border: 1px solid #ddd; padding: 10px; }}
            .plot img {{ max-width: 100%; height: auto; }}
            .footer {{ margin-top: 50px; font-size: 0.8em; color: #7f8c8d; }}
        </style>
    </head>
    <body>
        <h1>Relatório de Testes TCP vs UDP</h1>
        <p>Gerado em: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        
        <h2>Comparação de Desempenho</h2>
        <div class="plot">
            <h3>Tempo de Transferência</h3>
            <img src="transfer_time_comparison.png" alt="Tempo de Transferência">
        </div>
        
        <div class="plot">
            <h3>Velocidade de Transferência</h3>
            <img src="transfer_speed_comparison.png" alt="Velocidade de Transferência">
        </div>
        
        <h2>Análise de Distribuição</h2>
        <div class="plot">
            <h3>Distribuição das Velocidades</h3>
            <img src="speed_distribution.png" alt="Distribuição das Velocidades">
        </div>
        
        <div class="plot">
            <h3>Relação Tamanho-Velocidade</h3>
            <img src="size_vs_speed.png" alt="Relação Tamanho-Velocidade">
        </div>
    """

    # Adiciona gráfico de perda de pacotes se existir
    if os.path.exists(os.path.join(plots_dir, 'udp_packet_loss.png')):
        html_content += """
        <h2>Análise UDP</h2>
        <div class="plot">
            <h3>Perda de Pacotes UDP</h3>
            <img src="udp_packet_loss.png" alt="Perda de Pacotes UDP">
        </div>
        """

    # Adiciona seção 3D se existir
    if os.path.exists(os.path.join(plots_dir, '3d_interactive_plot.html')):
        html_content += """
        <h2>Visualização 3D Interativa</h2>
        <div class="plot">
            <iframe src="3d_interactive_plot.html" width="100%" height="600px" frameborder="0"></iframe>
        </div>
        """

    html_content += f"""
        <div class="footer">
            <p>Relatório gerado automaticamente pelo sistema de análise de desempenho</p>
        </div>
    </body>
    </html>
    """
    
    # Salva o arquivo HTML
    with open(os.path.join(results_dir, 'performance_report.html'), 'w') as f:
        f.write(html_content)

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) != 2:
        print("Uso: python visualize_results.py <diretorio_de_resultados>")
        sys.exit(1)
    
    results_dir = sys.argv[1]
    
    if not os.path.exists(results_dir):
        print(f"Diretório não encontrado: {results_dir}")
        sys.exit(1)
    
    print(f"Processando resultados em: {results_dir}")
    generate_all_plots(results_dir)
    print(f"Relatório e gráficos gerados em: {os.path.join(results_dir, 'visualizations')}")
    print(f"Relatório HTML disponível em: {os.path.join(results_dir, 'performance_report.html')}")