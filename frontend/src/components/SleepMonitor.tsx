import type { SensorStats } from '../types';

interface SleepMonitorProps {
  temp: SensorStats | null;
  sleepDuration: number;
}

interface StatCardProps {
  title: string;
  icon: string;
  stats: SensorStats | null;
  unit: string;
}

function StatCard({ title, icon, stats, unit }: StatCardProps) {
  if (!stats) {
    return (
      <div className="stat-card">
        <div className="stat-header">
          <span className="stat-icon">{icon}</span>
          <h3>{title}</h3>
        </div>
        <div className="stat-value">
          <span className="loading">Waiting for data...</span>
        </div>
      </div>
    );
  }

  return (
    <div className="stat-card">
      <div className="stat-header">
        <span className="stat-icon">{icon}</span>
        <h3>{title}</h3>
      </div>
      <div className="stat-value">
        <div className="current-value">
          {stats.last.toFixed(1)}<span className="unit">{unit}</span>
        </div>
      </div>
      <div className="stat-details">
        <div className="stat-item">
          <span className="stat-label">Average:</span>
          <span className="stat-number">{stats.avg.toFixed(1)}{unit}</span>
        </div>
        <div className="stat-item">
          <span className="stat-label">Min:</span>
          <span className="stat-number">{stats.min.toFixed(1)}{unit}</span>
        </div>
        <div className="stat-item">
          <span className="stat-label">Max:</span>
          <span className="stat-number">{stats.max.toFixed(1)}{unit}</span>
        </div>
      </div>
    </div>
  );
}

export function SleepMonitor({ temp, sleepDuration }: SleepMonitorProps) {
  // Format sleep duration to HH:MM:SS
  const formatDuration = (totalSeconds: number) => {
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = Math.floor(totalSeconds % 60);
    return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
  };

  return (
    <div className="card sleep-monitor">
      <h2>Sleep Monitor</h2>
      <p className="subtitle">Monitoring sleep duration and temperature</p>

      <div className="sleep-timer">
        <div className="timer-icon">😴</div>
        <div className="timer-label">Sleep Duration</div>
        <div className="timer-value">{formatDuration(sleepDuration)}</div>
      </div>

      <div className="stats-grid">
        <StatCard
          title="Body Temperature"
          icon="🌡️"
          stats={temp}
          unit="°C"
        />
      </div>

      {temp && (
        <div className="info-text" style={{ marginTop: '1rem', fontSize: '0.875rem' }}>
          <p>Temperature readings are from the thermistor sensor.</p>
          <p>Normal range: 20-30°C (ambient) or 35-38°C (body contact)</p>
        </div>
      )}
    </div>
  );
}
