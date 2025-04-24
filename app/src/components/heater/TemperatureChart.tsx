import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ReferenceLine,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

export default function TemperatureChart({
  graphData,
  targetTemperature,
}: Props) {
  return (
    <ResponsiveContainer width="100%" height="100%">
      <LineChart
        data={graphData}
        margin={{ bottom: 20, right: 20, left: 20, top: 20 }}
      >
        <CartesianGrid vertical={false} strokeDasharray="3" />
        <ReferenceLine y={targetTemperature} stroke="white" />
        <XAxis
          dataKey="seconds_elapsed"
          type="number"
          tickFormatter={(tick) => tick.toFixed(2).replace(/\.?0+$/, "")}
          tickCount={graphData.length}
          allowDecimals={false}
          domain={[0, "dataMax"]}
          label={{
            value: "Tempo",
            position: "bottom",
          }}
        />

        <YAxis
          domain={[0, 110]}
          label={{
            value: "Temperatura (°C)",
            angle: -90,
            position: "insideLeft",
          }}
        />
        <Tooltip animationDuration={100} />
        <Legend verticalAlign="top" />
        <Line
          name="Temperatura"
          dataKey="temperature"
          type="monotone"
          stroke="#8884d8"
          strokeWidth="3px"
          isAnimationActive={false}
          activeDot={{ r: 8 }}
        />
      </LineChart>
    </ResponsiveContainer>
  );
}

interface Props {
  graphData: GraphPoint[];
  targetTemperature: number | undefined;
}

export interface GraphPoint {
  temperature: number;
  seconds_elapsed: number;
}
