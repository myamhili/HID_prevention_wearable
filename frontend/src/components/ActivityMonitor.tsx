import type { Activity } from '../types';

interface ActivityMonitorProps {
  activity: Activity;
  seconds: number;
  maxSeconds: number;
  alert: boolean;
  warning: string;
}

export function ActivityMonitor({ activity, seconds, maxSeconds, alert, warning }: ActivityMonitorProps) {
  const percentage = maxSeconds > 0 ? (seconds / maxSeconds) * 100 : 0;

  // Determine color based on seconds level with tiered warnings
  let meterColor = '#22c55e'; // green - normal
  if (percentage <= 0) {
    meterColor = '#ef4444'; // red - last warning (0%)
  } else if (percentage <= 10) {
    meterColor = '#ff4500'; // red-orange - warning 2 (10%)
  } else if (percentage <= 30) {
    meterColor = '#ffa500'; // orange - warning 1 (30%)
  }

  const activityEmoji = {
    still: '🧍',
    active: '🚶',
    '...': '⏳',
  };

  // Format seconds to readable time (MM:SS)
  const formatTime = (totalSeconds: number) => {
    const mins = Math.floor(totalSeconds / 60);
    const secs = Math.floor(totalSeconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
  };

  return (
    <div className="card activity-monitor">
      <h2>Activity Monitor</h2>

      {alert && (
        <div className="alert-banner">
          ⚠️ Time to move! Inactivity detected.
        </div>
      )}

      {warning && !alert && (
        <div className="alert-banner" style={{ backgroundColor: meterColor }}>
          ⚠️ {warning === 'WARN1' ? 'Warning: Low activity time (30%)' : 'Critical: Very low activity time (10%)'}
        </div>
      )}

      <div className="activity-display">
        <div className="activity-icon">
          {activityEmoji[activity]}
        </div>
        <div className="activity-label">
          Current Activity: <strong>{activity}</strong>
        </div>
      </div>

      <div className="meter-container">
        <div className="meter-label">
          Activity Time Remaining
        </div>
        <div className="meter-bar-background">
          <div
            className="meter-bar-fill"
            style={{
              width: `${percentage}%`,
              backgroundColor: meterColor,
            }}
          />
        </div>
        <div className="meter-value">
          {formatTime(seconds)} / {formatTime(maxSeconds)} ({Math.round(percentage)}%)
        </div>
      </div>

      <div className="info-text">
        {seconds <= 0 ? (
          <p className="warning-text">⚠️ Activity time depleted - movement needed!</p>
        ) : activity === 'still' ? (
          <p>Still/inactive - time decreasing</p>
        ) : activity === 'active' ? (
          <p>Active movement detected - time recovering!</p>
        ) : (
          <p>Waiting for activity data...</p>
        )}
      </div>
    </div>
  );
}
