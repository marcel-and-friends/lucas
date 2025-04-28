import {
  ChartConfig,
  ChartContainer,
  ChartLegend,
  ChartLegendContent,
  ChartTooltip,
  ChartTooltipContent,
} from "@/components/ui/chart";
import {
  CartesianGrid,
  Line,
  LineChart,
  ReferenceLine,
  XAxis,
  YAxis,
} from "recharts";

const chartConfig = {
  temperature: {
    label: "Temperatura",
  },
} satisfies ChartConfig;

export default function TemperatureChart({
  graphData,
  targetTemperature,
}: Props) {
  return (
    <ChartContainer config={chartConfig} className="h-full w-full">
      <LineChart data={graphData}>
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
            position: "insideBottom",
          }}
        />
        <YAxis domain={[0, 110]} />
        <ChartTooltip
          animationDuration={100}
          cursor={false}
          content={<ChartTooltipContent hideLabel />}
        />
        <ChartLegend verticalAlign="top" content={<ChartLegendContent />} />
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
    </ChartContainer>
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
