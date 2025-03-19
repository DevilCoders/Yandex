package yandex.cloud.dashboard.generator;

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Streams;
import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.log4j.Log4j2;
import yandex.cloud.dashboard.integration.conductor.ConductorClient;
import yandex.cloud.dashboard.integration.conductor.ConductorClient.Node;
import yandex.cloud.dashboard.model.result.dashboard.Annotation;
import yandex.cloud.dashboard.model.result.dashboard.Annotations;
import yandex.cloud.dashboard.model.result.dashboard.Dashboard;
import yandex.cloud.dashboard.model.result.dashboard.Link;
import yandex.cloud.dashboard.model.result.dashboard.Options;
import yandex.cloud.dashboard.model.result.dashboard.Refresh;
import yandex.cloud.dashboard.model.result.dashboard.TemplateVariable;
import yandex.cloud.dashboard.model.result.dashboard.TemplateVariable.Item;
import yandex.cloud.dashboard.model.result.dashboard.Templating;
import yandex.cloud.dashboard.model.result.dashboard.Timepicker;
import yandex.cloud.dashboard.model.result.generic.GridPos;
import yandex.cloud.dashboard.model.result.generic.RGBA;
import yandex.cloud.dashboard.model.result.generic.RGBColor;
import yandex.cloud.dashboard.model.result.panel.DashboardList;
import yandex.cloud.dashboard.model.result.panel.Graph;
import yandex.cloud.dashboard.model.result.panel.Legend;
import yandex.cloud.dashboard.model.result.panel.Panel;
import yandex.cloud.dashboard.model.result.panel.Row;
import yandex.cloud.dashboard.model.result.panel.RowOrPanel;
import yandex.cloud.dashboard.model.result.panel.SeriesOverride;
import yandex.cloud.dashboard.model.result.panel.Target;
import yandex.cloud.dashboard.model.result.panel.Text;
import yandex.cloud.dashboard.model.result.panel.Tooltip;
import yandex.cloud.dashboard.model.result.panel.Xaxis;
import yandex.cloud.dashboard.model.result.panel.Yaxis;
import yandex.cloud.dashboard.model.result.panel.YaxisInList;
import yandex.cloud.dashboard.model.result.panel.singlestat.Gauge;
import yandex.cloud.dashboard.model.result.panel.singlestat.MappingType;
import yandex.cloud.dashboard.model.result.panel.singlestat.RangeMap;
import yandex.cloud.dashboard.model.result.panel.singlestat.Singlestat;
import yandex.cloud.dashboard.model.result.panel.singlestat.Sparkline;
import yandex.cloud.dashboard.model.result.panel.singlestat.ValueMap;
import yandex.cloud.dashboard.model.spec.Resolvable;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.dashboard.ConductorSpec;
import yandex.cloud.dashboard.model.spec.dashboard.ConductorSpec.Mode;
import yandex.cloud.dashboard.model.spec.dashboard.DashboardSpec;
import yandex.cloud.dashboard.model.spec.dashboard.DashboardSpec.PointerSharing;
import yandex.cloud.dashboard.model.spec.dashboard.TimeSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.DsVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.ExplicitVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.RepeatVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.UiQueryVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.UiVarSpec;
import yandex.cloud.dashboard.model.spec.generic.ColorSpec;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;
import yandex.cloud.dashboard.model.spec.generic.LabelsSpec;
import yandex.cloud.dashboard.model.spec.generic.QueryParamsSpec;
import yandex.cloud.dashboard.model.spec.generic.RGBASpec;
import yandex.cloud.dashboard.model.spec.panel.DashboardListSpec;
import yandex.cloud.dashboard.model.spec.panel.DashboardListSpec.SearchSpec;
import yandex.cloud.dashboard.model.spec.panel.DisplaySpec;
import yandex.cloud.dashboard.model.spec.panel.DisplaySpec.LineModesSpec.LineMode;
import yandex.cloud.dashboard.model.spec.panel.DrawSpec;
import yandex.cloud.dashboard.model.spec.panel.DrawSpec.At;
import yandex.cloud.dashboard.model.spec.panel.DrillDownSpec;
import yandex.cloud.dashboard.model.spec.panel.FunctionParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;
import yandex.cloud.dashboard.model.spec.panel.GroupByTimeSpec;
import yandex.cloud.dashboard.model.spec.panel.PanelSpec;
import yandex.cloud.dashboard.model.spec.panel.PlaceholderSpec;
import yandex.cloud.dashboard.model.spec.panel.QuerySpec;
import yandex.cloud.dashboard.model.spec.panel.RowSpec;
import yandex.cloud.dashboard.model.spec.panel.SinglestatSpec;
import yandex.cloud.dashboard.model.spec.panel.SsColoringSpec;
import yandex.cloud.dashboard.model.spec.panel.SsGaugeSpec;
import yandex.cloud.dashboard.model.spec.panel.SsSparklineSpec;
import yandex.cloud.dashboard.model.spec.panel.SsValueSpec;
import yandex.cloud.dashboard.model.spec.panel.template.Template;
import yandex.cloud.dashboard.model.spec.panel.template.TemplatedPanelSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;
import yandex.cloud.dashboard.model.spec.validator.Validatable;
import yandex.cloud.dashboard.util.Json;
import yandex.cloud.dashboard.util.Mergeable;
import yandex.cloud.dashboard.util.ObjectUtils;
import yandex.cloud.dashboard.util.reflection.ImmutableTransformer;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Queue;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.Stream.Builder;

import static com.google.common.base.MoreObjects.firstNonNull;
import static java.util.stream.Collectors.toList;
import static yandex.cloud.dashboard.model.spec.Constants.FORMAT_SHORT;
import static yandex.cloud.dashboard.model.spec.Constants.UI_VAR_ALL_TITLE;
import static yandex.cloud.dashboard.model.spec.Constants.UI_VAR_ALL_VALUE;
import static yandex.cloud.dashboard.util.Mergeable.mergeNullable;
import static yandex.cloud.dashboard.util.ObjectUtils.addToList;
import static yandex.cloud.dashboard.util.ObjectUtils.mapList;
import static yandex.cloud.dashboard.util.ObjectUtils.mapOrDefault;
import static yandex.cloud.dashboard.util.ObjectUtils.mapOrNull;
import static yandex.cloud.dashboard.util.ObjectUtils.negate;
import static yandex.cloud.util.BetterCollectors.throwingMerger;

/**
 * @author ssytnik
 */
@Log4j2
public class Generator {
    private static final long SCHEMA_VERSION = 16;

    private final Queue<DashboardSpec> queue;
    private final List<FolderAndDashboard> result;
    private DashboardSpec dashboardSpec;

    public Generator(DashboardSpec rootSpec) {
        queue = new ArrayDeque<>();
        queue.add(rootSpec);
        result = new ArrayList<>();
    }

    public List<FolderAndDashboard> generateAll() {
        int n = 0;
        while (!queue.isEmpty()) {
            dashboardSpec = queue.remove();
            log.info("Processing dashboard spec #{} ('{}')...", (n++), dashboardSpec.getTitle());
            result.add(new FolderAndDashboard(dashboardSpec.getFolderId(), generate()));
        }
        return result;
    }

    private Dashboard generate() {
        resolveReplacementVariables();
        Validatable.validateAll(dashboardSpec, new SpecValidationContext(null));

        Dashboard dashboard = createDashboard();
        List<RowOrPanel> panels = dashboard.getPanels();

        IdCursor id = new IdCursor();
        LayoutCursor layout = new LayoutCursor();

        if (dashboardSpec.getPanels() != null) {
            panels.addAll(createPanels(
                    dashboardSpec.getPanels(), id, layout,
                    dashboardSpec.getGraphDefaults(),
                    dashboardSpec.getQueryDefaults()));
        } else {
            firstNonNull(dashboardSpec.getRows(), List.<RowSpec>of()).forEach(rowSpec -> {
                String repeatName = rowSpec.getRepeat();
                if (repeatName == null) {
                    processRow(rowSpec, panels, id, layout);
                } else {
//                    rowSpec = rowSpec.withRepeat(null);
                    RepeatVarSpec repeatVarSpec = dashboardSpec.getRepeatVariablesSafe().get(repeatName);
                    ResolvedRepeatVarSpec resolved = new ResolvedRepeatVarSpec(repeatVarSpec);
                    for (int i = 0; i < resolved.getValues().size(); i++) {
                        log.info("* repeating row '{}' for repeat variable '{} = {}'", rowSpec.getTitle(), repeatName, resolved.getValues().get(i));
                        Map<String, String> replacementMap = createReplacementMap(repeatName, resolved, i);
                        RowSpec curRowSpec = Resolvable.resolve(rowSpec, replacementMap);
                        processRow(curRowSpec, panels, id, layout);
                    }
                }
            });
        }

        return dashboard;
    }

    private void processRow(RowSpec rowSpec,
                            List<RowOrPanel> panels,
                            IdCursor id,
                            LayoutCursor layout) {
        firstNonNull(rowSpec.getDrilldowns(), Collections.<DrillDownSpec>emptyList()).forEach(ddSpec ->
                queue.add(createDrilldownDashboardSpec(rowSpec, ddSpec)));

        Row row = createRow(id.advance(), rowSpec, layout.advance(24, 1));
        panels.add(row);
        if (firstNonNull(rowSpec.getCollapsed(), false)) {
            // follow Grafana issue: collapsed row panels' layout won't affect panels below that row
            row.setPanels(createRowPanels(rowSpec, id, layout.copy()));
        } else {
            panels.addAll(createRowPanels(rowSpec, id, layout));
        }
    }

    private List<Panel> createRowPanels(RowSpec rowSpec, IdCursor id, LayoutCursor layout) {
        return createPanels(rowSpec.getPanelsSafe(), id, layout,
                mergeNullable(rowSpec.getGraphDefaults(), dashboardSpec.getGraphDefaults()),
                mergeNullable(rowSpec.getQueryDefaults(), dashboardSpec.getQueryDefaults()));
    }

    private void resolveReplacementVariables() {
        Map<String, String> replacements = ObjectUtils.modifyWhileChanging(
                dashboardSpec.getReplacementVariablesSafe(), m -> Resolvable.resolve(m, m));
        dashboardSpec = Resolvable.resolve(dashboardSpec, replacements);
    }

    private Dashboard createDashboard() {
        return new Dashboard(
                new Annotations(new Annotation(
                        1,
                        "-- Grafana --",
                        true,
                        true,
                        new RGBA(0, 211, 255, 1),
                        "Annotations & Alerts",
                        "dashboard"
                )),
                true,
                null,
                firstNonNull(dashboardSpec.getPointerSharing(), PointerSharing.line).ordinal(),
                dashboardSpec.getId(),
                null,
                mapList(dashboardSpec.getAllLinks(), Link::createFromDashboardLink),
                new ArrayList<>(),
                new Refresh(dashboardSpec.getRefresh()),
                SCHEMA_VERSION,
                "dark",
                firstNonNull(dashboardSpec.getTags(), List.of()),
                new Templating(createTemplatingVariables()),
                firstNonNull(dashboardSpec.getTime(), TimeSpec.DEFAULT).asTime(),
                new Timepicker(
                        List.of("5s", "10s", "15s", "30s", "1m", "5m", "15m", "30m", "1h", "2h", "1d"),
                        List.of("5m", "15m", "1h", "6h", "12h", "24h", "2d", "7d", "30d")),
                "",
                dashboardSpec.getTitle(),
                dashboardSpec.getUid(),
                -1 // TODO version
        );
    }

    private DashboardSpec createDrilldownDashboardSpec(RowSpec rowSpec, DrillDownSpec ddSpec) {
        return new DashboardSpec(
                dashboardSpec.getFolderId(),
                null,
                ddSpec.resolveUid(dashboardSpec.getUid()),
                dashboardSpec.getTime(),
                dashboardSpec.getRefresh(),
                firstNonNull(ddSpec.getTitle(),
                        dashboardSpec.getTitle() + " - " + rowSpec.getTitle() + " - " + ddSpec.getUiRepeat() + " (rows)"),
                ddSpec.toVariablesSpec(dashboardSpec),
                PointerSharing.line,
                List.of(dashboardSpec.getAutoDrilldownLink()),
                addToList(ddSpec.getTags(), dashboardSpec.getAutoDrilldownTag()),
                dashboardSpec.getGraphDefaults(),
                dashboardSpec.getQueryDefaults(),
                null,
                List.of(rowSpec
                        .withDrilldowns(null)
                        .withRepeat(null)
                        .withUiRepeat(ddSpec.getUiRepeat())
                        .withTitle(rowSpec.getTitle() + " - $" + ddSpec.getUiRepeat())
                        .withPanels(new LabelsTransformer(ddSpec.getLabels()).transform(rowSpec.getPanels()))
                )
        );
    }

    @AllArgsConstructor
    private static class LabelsTransformer extends ImmutableTransformer<LabelsSpec> {
        private final LabelsSpec labels;

        @Override
        protected Class<LabelsSpec> applyTransformClass() {
            return LabelsSpec.class;
        }

        @Override
        protected LabelsSpec applyTransform(LabelsSpec source) {
            return Mergeable.mergeNullable(labels, source);
        }

        @Override
        protected Class beanTransformClass() {
            return Spec.class;
        }
    }

    private Row createRow(int id, RowSpec rowSpec, GridPos gridPos) {
        return new Row(
                id,
                gridPos,
                rowSpec.getTitle(),
                "row",
                firstNonNull(rowSpec.getCollapsed(), false),
                rowSpec.getUiRepeat(),
                List.of()
        );
    }

    private List<TemplateVariable> createTemplatingVariables() {
        return Streams.concat(
                dashboardSpec.getAllDsVariablesSafe().entrySet().stream().map(e -> createTemplateVariable(e.getKey(), e.getValue())),
                dashboardSpec.getAllUiVariablesSafe().entrySet().stream().map(e -> createTemplateVariable(e.getKey(), e.getValue())),
                dashboardSpec.getAllUiQueryVariablesSafe().entrySet().stream().map(e -> createTemplateVariable(e.getKey(), e.getValue()))
        ).collect(toList());
    }

    private static TemplateVariable createTemplateVariable(String name, DsVarSpec spec) {
        return new TemplateVariable(
                null,
                null, // NOTE canonical value: selected: true; text == value == datasource name
                firstNonNull(spec.getHidden(), false) ? 2 : 0,
                false,
                null, // NOTE canonical value: ""
                false,
                name,
                List.of(),
                spec.getType(), // NOTE canonical variable also included "queryValue": ""
                false,
                "datasource",
                null,
                null, // NOTE canonical value: 1
                mapOrNull(spec.getRegex(), regex -> String.format("/%s/", regex)),
                1 // sort: alphabetical (asc); NOTE canonical value: absent
        );
    }

    private static TemplateVariable createTemplateVariable(String name, UiVarSpec spec) {
        ResolvedVarSpec resolved = new ResolvedVarSpec(spec);
        List<String> values = resolved.getValues();
        List<String> titles = resolved.getTitles();
        Boolean multi = firstNonNull(spec.getMulti(), false);

        List<Item> items = new ArrayList<>();
        if (multi) {
            items.add(new Item(UI_VAR_ALL_TITLE, UI_VAR_ALL_VALUE));
        }
        Streams.zip(titles.stream(), values.stream(), Item::new).forEach(items::add);
        items.get(0).setSelected(true);

        return new TemplateVariable(
                null, // this seems unsupported/needless: (multi ? "All" : null)
                items.get(0),
                firstNonNull(spec.getHidden(), false) ? 2 : 0,
                multi,
                null,
                multi,
                name,
                items,
                String.join(",", values),
                false,
                "custom",
                null,
                null,
                null,
                null
        );
    }

    private TemplateVariable createTemplateVariable(String name, UiQueryVarSpec spec) {
        String datasource = mergeNullable(new GraphParamsSpec(spec.getDatasource(), null, null), dashboardSpec.getGraphDefaults()).getDatasource();
        QueryParamsSpec specToInheritLabels = firstNonNull(spec.getInheritLabels(), true) ? dashboardSpec.getQueryDefaults() : null;
        LabelsSpec labels = mergeNullable(new QueryParamsSpec(spec.getLabels(), null, null), specToInheritLabels).getLabels();
        Boolean multi = firstNonNull(spec.getMulti(), false);
        List<Item> items = multi ? List.of(new Item(UI_VAR_ALL_TITLE, UI_VAR_ALL_VALUE)) : List.of();
        return new TemplateVariable(
                spec.getIncludeAllValue(), // this seems unsupported/needless: (multi ? "All" : null)
                items.isEmpty() ? null : items.get(0),
                firstNonNull(spec.getHidden(), false) ? 2 : 0,
                !firstNonNull(spec.getIncludeAllValue(), "").isBlank() || multi,
                null,
                multi,
                name,
                items,
                labels.getSerializedSelector(true) + " #" + firstNonNull(spec.getLabel(), name),
                false,
                "query",
                datasource,
                2, // refresh: on time range change
                mapOrNull(spec.getRegex(), regex -> String.format("/%s/", regex)),
                1 // sort: alphabetical (asc)
        );
    }

    private List<Panel> createPanels(List<PanelSpec> panelSpecs, IdCursor id, LayoutCursor layout, GraphParamsSpec graphDefaults, QueryParamsSpec queryDefaults) {
        return panelSpecs.stream()
                .peek(panelSpec -> log.info("Processing {} panel '{}'...", panelSpec.getClass().getSimpleName(), panelSpec.getTitle()))
                .flatMap(panelSpec -> createPanels(panelSpec, id, layout, graphDefaults, queryDefaults))
                .collect(toList());
    }

    private Stream<Panel> createPanels(PanelSpec panelSpec, IdCursor id, LayoutCursor layout, GraphParamsSpec graphDefaults, QueryParamsSpec queryDefaults) {
        return PanelCreator.of(panelSpec.withParams(mergeNullable(panelSpec.getParams(), graphDefaults)))
                .bind(DashboardListSpec.class, spec -> Stream.of(createDashboardList(id.advance(), spec, layout.advance(spec.getParams()))))
                .bind(PlaceholderSpec.class, spec -> Stream.of(createPlaceholder(id.advance(), spec.getDescription(), layout.advance(spec.getParams()))))
                .bind(SinglestatSpec.class, spec -> {
                    spec = spec
                            .withQuery(spec.getQuery().withParams(mergeNullable(spec.getQuery().getParams(), queryDefaults)));

                    return resolvePanelSpec(spec).map(s -> createSingleStat(id.advance(), s, layout.advance(s.getParams())));
                })
                .bind(GraphSpec.class, spec -> {
                    QueryParamsSpec mergedQueryDefaults = mergeNullable(spec.getQueryDefaults(), queryDefaults);
                    spec = spec
                            .withQueries(firstNonNull(spec.getQueries(), List.<QuerySpec>of()).stream()
                                    .map(qs -> qs.withParams(mergeNullable(qs.getParams(), mergedQueryDefaults)))
                                    .collect(Collectors.toList()));

                    return resolvePanelSpec(spec).map(s -> createGraphOrPlaceholder(id.advance(), s, layout.advance(s.getParams())));
                })
                .create();
    }

    private <T extends Template<S, T>, S extends TemplatedPanelSpec<S, T>> Stream<S> resolvePanelSpec(S spec) {
        GraphParamsSpec.validateResolveDatasource(spec.getParams());

        spec = resolveTemplates(spec);

        String repeatName = spec.getRepeat();
        if (repeatName == null) {
            return Stream.of(spec);
        } else {
//            spec = spec.withRepeat(null);
            RepeatVarSpec repeatVarSpec = dashboardSpec.getRepeatVariablesSafe().get(repeatName);

            ResolvedRepeatVarSpec resolved = new ResolvedRepeatVarSpec(repeatVarSpec);
            Builder<S> builder = Stream.builder();
            for (int i = 0; i < resolved.getValues().size(); i++) {
                log.info("* repeating graph '{}' for repeat variable '{} = {}'", spec.getTitle(), repeatName, resolved.getValues().get(i));
                builder.add(Resolvable.resolve(spec, createReplacementMap(repeatName, resolved, i)));
            }
            return builder.build();
        }
    }

    private Map<String, String> createReplacementMap(String repeatName, ResolvedRepeatVarSpec resolved, int index) {
        String repeatValue = resolved.getValues().get(index);
        String repeatTitle = resolved.getTitles().get(index);
        Map<String, List<String>> repeatVariables = resolved.getVariables();

        ImmutableMap.Builder<String, String> replacementMap = ImmutableMap.<String, String>builder()
                .put(repeatName, repeatValue);
        if (!repeatVariables.containsKey("title")) {
            replacementMap.put(repeatName + ":title", repeatTitle);
        }
        repeatVariables.forEach((key, values) -> replacementMap.put(repeatName + ":" + key, values.get(index)));
        return replacementMap.build();
    }

    private DashboardList createDashboardList(int id, DashboardListSpec dlSpec, GridPos gridPos) {
        SearchSpec search = dlSpec.getSearch();
        return new DashboardList(
                id,
                gridPos,
                dlSpec.getTitle(),
                dlSpec.getDescription(),
                "dashlist",
                mapOrDefault(search, SearchSpec::getQuery, ""),
                firstNonNull(dlSpec.getLimit(), 10),
                mapOrDefault(search, SearchSpec::getTags, List.of()),
                firstNonNull(dlSpec.getRecent(), false),
                search != null,
                firstNonNull(dlSpec.getStarred(), false),
                firstNonNull(dlSpec.getHeadings(), false),
                mapOrNull(search, SearchSpec::getFolderId)
        );
    }

    private Panel createGraphOrPlaceholder(int id, GraphSpec graphSpec, GridPos gridPos) {
        if (graphSpec.isPlaceholderSpec()) {
            return createPlaceholder(id, graphSpec.getDescription(), gridPos);
        }

        Graph graph = createGraph(id, graphSpec, graphSpec.getParams(), gridPos);

        int queryNumber = 1;
        for (QuerySpec querySpec : firstNonNull(graphSpec.getQueries(), List.<QuerySpec>of())) {
            graph.getTargets().add(createTargetFromQuery(querySpec, "Q" + queryNumber++));
        }

        return graph;
    }

    private Singlestat createSingleStat(int id, SinglestatSpec ssSpec, GridPos gridPos) {
        SsColoringSpec ssColoringSpec = mergeNullable(ssSpec.getColoring(), SsColoringSpec.DEFAULT);
        SsValueSpec ssValueSpec = mergeNullable(ssSpec.getValue(), SsValueSpec.DEFAULT);

        Singlestat ss = new Singlestat(
                id,
                gridPos,
                ssSpec.getTitle(),
                ssSpec.getDescription(),
                "singlestat",
                null,
                ssColoringSpec.getColorBackground(),
                ssColoringSpec.getColorPostfix(),
                ssColoringSpec.getColorPrefix(),
                ssColoringSpec.getColorValue(),
                ssColoringSpec.getColorsList().stream().map(this::rgbColor).collect(toList()),
                ssSpec.getParams().getDatasource(),
                ssValueSpec.getDecimals(),
                firstNonNull(ssSpec.getFormat(), "none"),
                createGauge(mergeNullable(ssSpec.getGauge(), SsGaugeSpec.DEFAULT)),
                null,
                mapList(ssSpec.getLinks(), Link::createFromGraphLink, null),
                1,
                List.of(new MappingType("value to text", 1), new MappingType("range to text", 2)),
                1000,
                "connected",
                null,
                ssValueSpec.getPostfix(),
                ssValueSpec.getPostfixFontSize(),
                ssValueSpec.getPrefix(),
                ssValueSpec.getPrefixFontSize(),
                List.of(new RangeMap("null", "N/A", "null")),
                createSparkline(mergeNullable(ssSpec.getSparkline(), SsSparklineSpec.DEFAULT)),
                "",
                new ArrayList<>(), //???
                ssColoringSpec.getThresholds().stream().map(Objects::toString).collect(Collectors.joining(",")),
                null,
                null,
                ssValueSpec.getValueFontSize(),
                List.of(new ValueMap("=", "N/A", "null")),
                ssValueSpec.getValueFunction()
        );
        ss.getTargets().add(createTargetFromQuery(ssSpec.getQuery(), "Q"));

        return ss;
    }

    private Target createTargetFromQuery(QuerySpec query, String queryName) {
        QueryParamsSpec queryParams = query.getParams();

        Target target = new Target(
                new ArrayList<>(),
                null,
                queryName,
                List.of(new ArrayList<>()),
                null,
                null,
                "timeserie"
        );
        if (query.getExpr() != null) {
            Map<String, String> labelsReplacementMap = queryParams == null || queryParams.getLabels() == null ? Map.of() :
                    Map.of("labels", queryParams.getLabels().getSerializedSelector(false));
            target.setTarget(Resolvable.resolve(query.getExpr(), labelsReplacementMap));
        } else {
            target.setTags(queryParams.getLabels().getTags());
            target.initFieldValue(queryParams.getLabels());
            target.addGroupByTime(
                    firstNonNull(query.getGroupByTime(), GroupByTimeSpec.AVG_GRAFANA_AUTO),
                    queryParams.getDefaultTimeWindow()
            );
            if (firstNonNull(queryParams.getDropNan(), false)) {
                target.addFunctionCall("drop_nan", FunctionParamsSpec.NO_PARAMS);
            }
            if (query.getSelect() != null) {
                query.getSelect().forEach(target::addFunctionCall);
            }
        }
        return target;
    }

    private Sparkline createSparkline(SsSparklineSpec sparkline) {
        return new Sparkline(
                createRGBA(sparkline.getFillColor()),
                sparkline.getFullHeight(),
                rgbColor(sparkline.getLineColor()),
                sparkline.getShow()
        );
    }

    private RGBA createRGBA(RGBASpec fillColor) {
        return new RGBA(
                fillColor.getColor().getR(),
                fillColor.getColor().getG(),
                fillColor.getColor().getB(),
                fillColor.getAlpha());
    }

    private Gauge createGauge(SsGaugeSpec gauge) {
        return new Gauge(
                gauge.getMaxValue(),
                gauge.getMinValue(),
                gauge.getShow(),
                gauge.getThresholdLabels(),
                gauge.getThresholdMarkers());
    }

    private Text createPlaceholder(int id, String description, GridPos gridPos) {
        return new Text(
                id,
                gridPos,
                "",
                description,
                "text",
                "markdown",
                "",
                List.of());
    }

    private Graph createGraph(int id, GraphSpec graphSpec, GraphParamsSpec graphParams, GridPos gridPos) {
        DisplaySpec displaySpec = Mergeable.mergeNullable(graphSpec.getDisplay(), DisplaySpec.DEFAULT);

        return new Graph(
                id,
                gridPos,
                graphSpec.getTitle(),
                graphSpec.getDescription(),
                "graph",
                mapOrDefault(graphSpec.getDraw(), this::createAliasColors, Map.of()),
                displaySpec.containsLineMode(LineMode.bars),
                10,
                false,
                graphParams.getDatasource(),
                displaySpec.getDecimals(),
                firstNonNull(displaySpec.getFill(), 1),
                createLegend(displaySpec),
                displaySpec.containsLineMode(LineMode.lines),
                firstNonNull(displaySpec.getLineWidth(), 1),
                mapList(graphSpec.getLinks(), Link::createFromGraphLink, null),
                firstNonNull(displaySpec.getNulls(), DisplaySpec.NullPointMode.keep).serialize(),
                mapOrDefault(
                        mapList(graphSpec.getDataLinks(), Link::createFromGraphLink, null),
                        Options::new,
                        null),
                false,
                5,
                displaySpec.containsLineMode(LineMode.points),
                "flot",
                graphSpec.getUiRepeat(),
                (graphSpec.getUiRepeat() == null ? null : "h"),
                mapOrDefault(graphSpec.getDraw(), this::createSeriesOverrides, List.of()),
                10,
                displaySpec.getStack(),
                false,
                new ArrayList<>(), // note: default is null
                List.of(),
                null,
                List.of(),
                null,
                new Tooltip(true, displaySpec.getSort().getMode().getValue(), "individual"),
                new Xaxis(null, "time", null, true, List.of()),
                ObjectUtils.mapListOfSize(
                        2,
                        graphSpec.getYAxes(),
                        spec -> new YaxisInList(spec.getDecimals(), firstNonNull(spec.getFormat(), FORMAT_SHORT),
                                spec.getLabel(), firstNonNull(spec.getLogBase(), 1), spec.getMax(), spec.getMin(), true),
                        () -> new YaxisInList(null, FORMAT_SHORT, null, 1, null, null, true)
                ),
                new Yaxis(false, null)
        );
    }

    private <T extends Template<S, T>, S extends TemplatedPanelSpec<S, T>> S resolveTemplates(S spec) {
        if (spec.getTemplates() != null) {
            for (T template : flatten(spec.getTemplates().getList())) {
                log.info("* resolving " + template.getClass().getSimpleName() + "...");
                spec = template.transform(spec);
//                log.debug("* intermediate GraphSpec: " + Json.toJson(graphSpec));
            }
            spec = spec.withTemplates(null);
            log.debug("$ resolved GraphSpec: " + Json.toJson(spec));
        }
        return spec;
    }

    private <T extends Template<S, T>, S extends TemplatedPanelSpec<S, T>> List<T> flatten(List<T> l) {
        return l.stream()
                .flatMap(t -> flatten(t).stream())
                .collect(toList());
    }

    private <T extends Template<S, T>, S extends TemplatedPanelSpec<S, T>> List<T> flatten(T t) {
        return ImmutableList.<T>builder()
                .addAll(flatten(t.precedingTemplates()))
                .add(t)
                .addAll(flatten(t.successiveTemplates()))
                .build();
    }

    private Map<String, RGBColor> createAliasColors(List<DrawSpec> draw) {
        return draw.stream()
                .filter(d -> d.getColor() != null)
                .collect(Collectors.toMap(
                        DrawSpec::getAlias,
                        d -> rgbColor(d.getColor()),
                        throwingMerger(),
                        LinkedHashMap::new));
    }

    private RGBColor rgbColor(ColorSpec spec) {
        return new RGBColor(spec.getR(), spec.getG(), spec.getB());
    }

    private Legend createLegend(DisplaySpec ds) {
        Boolean hideEmpty = negate(ds.getEmpty());
        return new Legend(false, false, hideEmpty, hideEmpty, false, false, ds.getLegend().isRightSide(), ds.getLegend().isShow(), false, false);
    }

    // TODO support separate stacks for left and right axes, if stacking is enabled
    private List<SeriesOverride> createSeriesOverrides(List<DrawSpec> draw) {
        return draw.stream()
                .filter(d -> d.getAt() == At.right || d.getStack() != null)
                .map(d -> new SeriesOverride(d.getAlias(), d.getAt() == At.right ? 2 : null, d.getStack()))
                .collect(toList());
    }


    private static class IdCursor {
        private int id = 0;

        int advance() {
            return ++id;
        }
    }

    @AllArgsConstructor
    private static class LayoutCursor {
        int x;
        int y;
        int width;
        int height;
        int rowMaxHeight;

        public LayoutCursor() {
            this(0, 0, Integer.MIN_VALUE, Integer.MIN_VALUE, 0);
        }

        public LayoutCursor copy() {
            return new LayoutCursor(x, y, width, height, rowMaxHeight);
        }

        GridPos advance(GraphParamsSpec params) {
            GraphParamsSpec.validateResolvedSize(params);
            return advance(params.getWidth(), params.getHeight());
        }

        GridPos advance(int width, int height) {
            Preconditions.checkArgument(1 <= width && width <= 24, "'1 <= width <= 24' violation");
            Preconditions.checkArgument(1 <= height, "'1 <= height' violation");

            this.width = width;
            this.height = height;

            if (x + width > 24) {
                x = 0;
                y += rowMaxHeight;
                rowMaxHeight = 0;
            }

            GridPos gridPos = new GridPos(x, y, width, height);

            x += width;
            rowMaxHeight = Math.max(rowMaxHeight, height);

            return gridPos;
        }
    }

    @RequiredArgsConstructor(staticName = "of")
    static class PanelCreator {
        private final PanelSpec spec;
        private Stream<Panel> result = null;

        @SuppressWarnings("unchecked")
        <P extends PanelSpec> PanelCreator bind(Class<P> clazz, Function<P, Stream<Panel>> f) {
            if (clazz.isInstance(spec)) {
                result = f.apply((P) spec);
            }
            return this;
        }

        Stream<Panel> create() {
            return Preconditions.checkNotNull(result, "Unknown panel spec class: " + spec.getClass());
        }
    }

    @Value
    static class ResolvedVarSpec {
        List<String> values;
        List<String> titles;

        ResolvedVarSpec(ExplicitVarSpec spec) {
            List<String> values = firstNonNull(spec.getValues(), List.of());
            List<String> titles = firstNonNull(spec.getTitles(), values);
            ConductorSpec conductor = spec.getConductor();

            ImmutableList.Builder<String> vb = ImmutableList.<String>builder().addAll(values);
            ImmutableList.Builder<String> tb = ImmutableList.<String>builder().addAll(titles);

            if (conductor != null) {
                Mode mode = firstNonNull(conductor.getMode(), Mode.host);
                boolean fqdn = firstNonNull(conductor.getFqdn(), true);

                switch (mode) {
                    case host:
                        List<String> hosts = strip(fqdn, DefaultConductorClientHolder.INSTANCE.getHosts(conductor.getGroup()));
                        vb.addAll(hosts);
                        tb.addAll(hosts);
                        break;
                    case tree:
                        List<Node> tree = DefaultConductorClientHolder.INSTANCE.getTree(conductor.getGroup());
                        vb.addAll(tree.stream().map(n -> String.join("|", strip(fqdn, n.getHosts()))).collect(toList()));
                        // FIXME here, strip() is also applied to group names now
                        tb.addAll(strip(fqdn, tree.stream().map(Node::getName).collect(toList())));
                        break;
                    default:
                        throw new IllegalStateException("Unknown conductor mode: " + mode);
                }
            }

            this.values = vb.build();
            this.titles = tb.build();
        }

        static List<String> strip(boolean fqdn, List<String> fqdnHosts) {
            return fqdn ? fqdnHosts : fqdnHosts.stream().map(h -> h.replaceFirst("\\..+$", "")).collect(toList());
        }

        static class DefaultConductorClientHolder {
            static ConductorClient INSTANCE = new ConductorClient();
        }
    }

    @Value
    static class ResolvedRepeatVarSpec {
        ResolvedVarSpec baseSpec;
        Map<String, List<String>> variables;

        ResolvedRepeatVarSpec(RepeatVarSpec spec) {
            this.baseSpec = new ResolvedVarSpec(spec);
            this.variables = firstNonNull(spec.getVariables(), Map.of());
        }

        List<String> getValues() {
            return baseSpec.getValues();
        }

        List<String> getTitles() {
            return baseSpec.getTitles();
        }
    }
}
