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
import { HeatingStage } from "#/proto/firmware/HeatingReport";

const chartConfig = {
  temperature: {
    label: "Temperatura (°C)",
  },
  preheatTemperature: {
    label: "Temperatura - preheat (°C)",
  },
  pid: {
    label: "PID (%)",
  },
  watts: {
    label: "Força (%)",
  },
} satisfies ChartConfig;

export default function TemperatureChart({
  graphData,
  targetTemperature,
}: Props) {
  const firstHeating = graphData.findIndex(
    (d) => d.stage == HeatingStage.Heating,
  );

  let heatingPercentage = 0;
  if (firstHeating != -1) {
    heatingPercentage =
      ((graphData.length - firstHeating) * 100) / (graphData.length - 1);
  }

  return (
    <ChartContainer config={chartConfig}>
      <LineChart data={graphData}>
        <CartesianGrid vertical={false} strokeDasharray="3" />
        <ReferenceLine y={targetTemperature} stroke="white" />
        <XAxis
          dataKey="secondsElapsed"
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
        <YAxis
          type="number"
          domain={[20, 100]}
          allowDecimals={false}
          tickFormatter={(tick) => tick.toFixed(2).replace(/\.?0+$/, "")}
        />
        <ChartTooltip
          animationDuration={100}
          content={<ChartTooltipContent hideLabel hideIndicator />}
        />
        <ChartLegend verticalAlign="top" content={<ChartLegendContent />} />
        <defs>
          <linearGradient id="gradient" x1="0" y1="0" x2="100%" y2="0">
            {(() => {
              switch (heatingPercentage) {
                case 0:
                  return (
                    <>
                      <stop offset="0%" stopColor="#f58d42" />
                      <stop offset="100%" stopColor="#f58d42" />
                    </>
                  );
                case 100:
                  return (
                    <>
                      <stop offset="0%" stopColor="#8884d8" />
                      <stop offset="100%" stopColor="#8884d8" />
                    </>
                  );
                default:
                  return (
                    <>
                      <stop offset="0%" stopColor="#f58d42" />
                      <stop
                        offset={`${100 - heatingPercentage}%`}
                        stopColor="#f58d42"
                      />
                      <stop
                        offset={`${100 - heatingPercentage}%`}
                        stopColor="#8884d8"
                      />
                      <stop offset="100%" stopColor="#8884d8" />
                    </>
                  );
              }
            })()}
          </linearGradient>
        </defs>
        <Line
          name="Temperatura"
          dataKey="temperature"
          type="monotone"
          stroke="#8884d8"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
        />
        <Line
          name="Temperatura (preheat)"
          dataKey="preheatTemperature"
          type="monotone"
          stroke="#f58d42"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
        />
        <Line
          name="PID"
          dataKey="pid"
          type="monotone"
          stroke="#7dff45"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
        />
        <Line
          name="Força"
          dataKey="watts"
          type="monotone"
          stroke="#ff5485"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
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
  temperature: number | undefined;
  preheatTemperature: number | undefined;
  pid: number | undefined;
  watts: number;
  secondsElapsed: number;
  stage: HeatingStage;
}
