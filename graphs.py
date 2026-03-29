import matplotlib.pyplot as plt

# ---------------------------------------------------------
# 1. Data Input (Extracted from terminal screenshot)
# ---------------------------------------------------------
dataset_size_mb = 52.3393
stages = ['1. Encryption\n(Client)', '2. Blind Compute\n(Cloud)', '3. Decryption\n(Client)']
times = [0.011, 0.013, 0.034]  # In seconds

# Set a professional, clean style suitable for IEEE/ACM papers
plt.style.use('seaborn-v0_8-whitegrid')

# ---------------------------------------------------------
# Graph 1: Latency Breakdown (Bar Chart)
# ---------------------------------------------------------
fig1, ax1 = plt.subplots(figsize=(8, 6))
colors_bar = ['#e63946', '#457b9d', '#1d3557'] # Red, Light Blue, Dark Blue

bars = ax1.bar(stages, times, color=colors_bar, edgecolor='black', linewidth=1.2, width=0.6)

ax1.set_title(f'End-to-End Pipeline Latency\n(Dataset Size: {dataset_size_mb:.1f} MB)', 
              fontsize=14, fontweight='bold', pad=15)
ax1.set_ylabel('Time (Seconds)', fontsize=12, fontweight='bold')
ax1.set_ylim(0, max(times) + 0.01) # Add headroom for labels

# Add exact value labels on top of the bars
for bar in bars:
    yval = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2, yval + 0.001, 
             f'{yval:.3f}s', ha='center', va='bottom', fontsize=12, fontweight='bold')

plt.tight_layout()
plt.savefig('pipeline_latency_bar.png', dpi=300, bbox_inches='tight')
print("Graph 1 saved as 'pipeline_latency_bar.png'")
plt.close(fig1) # Close the figure so it doesn't overlap with the next one

# ---------------------------------------------------------
# Graph 2: Time Distribution (Pie Chart)
# ---------------------------------------------------------
fig2, ax2 = plt.subplots(figsize=(7, 7))
colors_pie = ['#ff9f1c', '#2ec4b6', '#011627'] # Orange, Teal, Navy

# Explode the 'Blind Compute' slice slightly to highlight it
explode = (0.05, 0.1, 0.05) 

ax2.pie(times, explode=explode, labels=stages, colors=colors_pie, autopct='%1.1f%%',
        shadow=False, startangle=140, textprops={'fontsize': 12, 'fontweight': 'bold'})

ax2.set_title('Latency Distribution Across Pipeline Stages', fontsize=14, fontweight='bold', pad=15)

plt.tight_layout()
plt.savefig('pipeline_latency_pie.png', dpi=300, bbox_inches='tight')
print("Graph 2 saved as 'pipeline_latency_pie.png'")
plt.close(fig2)

print("All graphs generated successfully!")