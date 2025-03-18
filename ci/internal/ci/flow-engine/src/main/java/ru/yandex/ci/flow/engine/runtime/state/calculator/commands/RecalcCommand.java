package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

/**
 * Команда, которая не вызывает побочных эффектов (вроде скедулинга джобы в базинге),
 * просто пересчитывает стейт флоу.
 */
public interface RecalcCommand extends PendingCommand {
}
